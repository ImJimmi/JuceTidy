#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/DenseMap.h>

namespace clang::tidy::juce
{
    namespace block_size_assumption
    {
        class BlockSizeAssumptionCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isInstantiated()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                auto& context = *result.Context;

                for (const auto& [prepareName, parameterIndex, callbackName] : getCallbackPairs())
                {
                    const auto findDefinitions = [record](const std::string& name) {
                        std::vector<const FunctionDecl*> definitions;

                        for (const auto* method : record->methods())
                        {
                            if (const FunctionDecl* definition = nullptr;
                                method->size_overridden_methods() > 0
                                && utils::isOrOverridesAnyOf(*method, { name })
                                && method->hasBody(definition))
                            {
                                definitions.push_back(definition);
                            }
                        }

                        return definitions;
                    };

                    BlockSizeAssignments assignments;

                    for (const auto* prepare : findDefinitions(prepareName))
                        assignments.insert_range(findBlockSizeAssignments(*prepare,
                                                                          parameterIndex,
                                                                          context));

                    if (assignments.empty())
                        continue;

                    for (const auto* callback : findDefinitions(callbackName))
                        reportUses(*callback, assignments, context);
                }
            }

        private:
            struct BlockSizeAssignment
            {
                const FunctionDecl* prepare;
                const BinaryOperator* assignment;
            };

            using BlockSizeAssignments = llvm::DenseMap<const FieldDecl*, BlockSizeAssignment>;

            struct CallbackPair
            {
                std::string prepare;
                unsigned int blockSizeParameter;
                std::string callback;
            };

            void reportUses(const FunctionDecl& callback,
                            const BlockSizeAssignments& assignments,
                            ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto fieldReference = memberExpr(member(fieldDecl().bind("field"))).bind("use");
                const auto juceCall = callExpr(callee(functionDecl(matchesName("^::juce::"))));
                const auto use = expr(anyOf(binaryOperator(isComparisonOperator(),
                                                           hasEitherOperand(ignoringParenImpCasts(fieldReference)),
                                                           hasParent(stmt(anyOf(forStmt(),
                                                                                whileStmt(),
                                                                                doStmt())))),
                                            callExpr(juceCall,
                                                     forEachArgumentWithParam(ignoringParenImpCasts(fieldReference),
                                                                              parmVarDecl()))));

                for (const auto& nodes : match(findAll(use),
                                               *callback.getBody(),
                                               context))
                {
                    const auto* field = nodes.getNodeAs<FieldDecl>("field");
                    const auto [prepare, assignment] = assignments.lookup(field);

                    if (assignment == nullptr)
                        continue;

                    diag(nodes.getNodeAs<Expr>("use")->getExprLoc(),
                         "the size of the buffer may be smaller than %0, whose "
                         "value was copied from the maximum buffer size passed "
                         "to %1. Check the actual size of the buffer here "
                         "instead")
                        << field
                        << prepare;

                    diag(assignment->getBeginLoc(),
                         "%0 is assigned the maximum buffer size here",
                         DiagnosticIDs::Note)
                        << field;
                }
            }

            [[nodiscard]] static const std::array<CallbackPair, 2>& getCallbackPairs()
            {
                static const std::array pairs{
                    CallbackPair{
                        .prepare = "juce::AudioProcessor::prepareToPlay",
                        .blockSizeParameter = 1,
                        .callback = "juce::AudioProcessor::processBlock",
                    },
                    CallbackPair{
                        .prepare = "juce::AudioSource::prepareToPlay",
                        .blockSizeParameter = 0,
                        .callback = "juce::AudioSource::getNextAudioBlock",
                    },
                };

                return pairs;
            }

            [[nodiscard]] static BlockSizeAssignments findBlockSizeAssignments(const FunctionDecl& prepare,
                                                                               unsigned int parameterIndex,
                                                                               ASTContext& context)
            {
                using namespace clang::ast_matchers;

                BlockSizeAssignments assignments;

                if (prepare.getNumParams() <= parameterIndex
                    || prepare.getBody() == nullptr)
                {
                    return assignments;
                }

                const auto* parameter = prepare.getParamDecl(parameterIndex);
                const auto assignment = binaryOperator(isAssignmentOperator(),
                                                       hasRHS(ignoringParenImpCasts(anyOf(declRefExpr(to(equalsNode(parameter))),
                                                                                          explicitCastExpr(hasSourceExpression(ignoringParenImpCasts(declRefExpr(to(equalsNode(parameter))))))))))
                                            .bind("assignment");

                for (const auto& nodes : match(findAll(assignment),
                                               *prepare.getBody(),
                                               context))
                {
                    if (const auto* matched = nodes.getNodeAs<BinaryOperator>("assignment");
                        const auto* field = utils::getReferencedField(*matched->getLHS()))
                    {
                        assignments.try_emplace(field,
                                                BlockSizeAssignment{
                                                    .prepare = &prepare,
                                                    .assignment = matched,
                                                });
                    }
                }

                return assignments;
            }
        };
    } // namespace block_size_assumption

    using block_size_assumption::BlockSizeAssumptionCheck;
} // namespace clang::tidy::juce
