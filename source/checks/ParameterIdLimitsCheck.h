#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>

namespace clang::tidy::juce
{
    namespace parameter_id_limits
    {
        class ParameterIdLimitsCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto parameterClass = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorParameterWithID")));
                const auto literal = ignoringParenImpCasts(stringLiteral().bind("id"));
                const auto enclosingFunction = forCallable(functionDecl().bind("function"));

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::ParameterID")))),
                                     hasArgument(0, literal),
                                     enclosingFunction),
                    this);

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(hasAnyName("::std::make_unique",
                                                            "::std::make_shared"),
                                                 hasTemplateArgument(0,
                                                                     refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(parameterClass))))))),
                             hasArgument(0, literal),
                             enclosingFunction),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* literal = result.Nodes.getNodeAs<StringLiteral>("id");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");

                if (!literal->isOrdinary()
                    || utils::isInMacroBody(literal->getBeginLoc(), *result.SourceManager))
                {
                    return;
                }

                const auto id = literal->getString();

                if (id.size() > maximumAaxParameterIdLength)
                {
                    diag(literal->getBeginLoc(),
                         "parameter ID '%0' is %1 characters long. IDs must be "
                         "at most 31 characters long")
                        << id
                        << static_cast<unsigned int>(id.size())
                        << static_cast<unsigned int>(maximumAaxParameterIdLength);
                }

                const auto [existing, inserted] = seenIds.try_emplace(function->getCanonicalDecl())
                                                      .first->second.try_emplace(id, literal->getBeginLoc());

                if (inserted)
                    return;

                diag(literal->getBeginLoc(),
                     "parameter ID '%0' is already used by another parameter")
                    << id;

                diag(existing->second,
                     "'%0' is first used here",
                     DiagnosticIDs::Note)
                    << id;
            }

            void onStartOfTranslationUnit() override
            {
                seenIds.clear();
            }

        private:
            static inline constexpr size_t maximumAaxParameterIdLength = 31;

            llvm::DenseMap<const Decl*, llvm::StringMap<SourceLocation>> seenIds;
        };
    } // namespace parameter_id_limits

    using parameter_id_limits::ParameterIdLimitsCheck;
} // namespace clang::tidy::juce
