#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unprepared_dsp
    {
        class UnpreparedDspCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 has(fieldDecl()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                std::vector<std::pair<const FieldDecl*, Preparation>> candidates;

                for (const auto* field : record->fields())
                {
                    if (const auto preparation = getPreparation(field->getType());
                        preparation && !field->getLocation().isMacroID())
                    {
                        candidates.emplace_back(field, *preparation);
                    }
                }

                if (candidates.empty())
                    return;

                const auto statements = utils::getClassStatements(*record);

                if (!statements)
                    return;

                for (const auto& [field, preparation] : candidates)
                {
                    if (isPreparedOrShared(*field, preparation.method, *statements, *result.Context))
                        continue;

                    diag(field->getLocation(),
                         "'%0' never has '%1()' called on it. Call '%0.%2' in "
                         "'prepareToPlay()' or elsewhere")
                        << field->getName()
                        << preparation.method
                        << preparation.example;
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            struct Preparation
            {
                StringRef method;
                StringRef example;
            };

            [[nodiscard]] static std::optional<Preparation> getPreparation(QualType type)
            {
                const auto* record = utils::getRecord(type);

                if (record == nullptr)
                    return std::nullopt;

                const auto name = utils::qualifiedName(*record);

                if (name == "juce::SmoothedValue" || name == "juce::LinearSmoothedValue")
                {
                    return Preparation{
                        .method = "reset",
                        .example = "reset(sampleRate, rampLengthInSeconds)",
                    };
                }

                if (name == "juce::ADSR" || name == "juce::Reverb")
                {
                    return Preparation{
                        .method = "setSampleRate",
                        .example = "setSampleRate(sampleRate)",
                    };
                }

                if (name == "juce::dsp::Oversampling")
                {
                    return Preparation{
                        .method = "initProcessing",
                        .example = "initProcessing(maximumBlockSize)",
                    };
                }

                if (StringRef(name).starts_with("juce::dsp::")
                    && utils::hasMethodNamed(record, "prepare"))
                {
                    return Preparation{
                        .method = "prepare",
                        .example = "prepare(spec)",
                    };
                }

                return std::nullopt;
            }

            [[nodiscard]] static bool isPreparedOrShared(const FieldDecl& field,
                                                         StringRef method,
                                                         ArrayRef<const Stmt*> statements,
                                                         ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = [&field](const Expr* expression) {
                    return expression != nullptr && utils::getReferencedField(*expression) == &field;
                };

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(cxxMemberCallExpr(callee(cxxMethodDecl(hasName(method))))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (references(nodes.getNodeAs<CXXMemberCallExpr>("call")->getImplicitObjectArgument()))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(callExpr().bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(nodes.getNodeAs<CallExpr>("call")->arguments(), references))
                            return true;
                    }
                }

                return false;
            }
        };
    } // namespace unprepared_dsp

    using unprepared_dsp::UnpreparedDspCheck;
} // namespace clang::tidy::juce
