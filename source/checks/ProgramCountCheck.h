#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>

namespace clang::tidy::juce
{
    namespace program_count
    {
        class ProgramCountCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    returnStmt(hasReturnValue(expr().bind("value")),
                               forCallable(cxxMethodDecl(unless(isInstantiated()),
                                                         utils::isOrOverridesAnyOf({ "juce::AudioProcessor::getNumPrograms" }))))
                        .bind("return"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* value = result.Nodes.getNodeAs<Expr>("value");

                if (utils::isInMacroBody(value->getExprLoc(), *result.SourceManager))
                    return;

                const auto count = value->getIntegerConstantExpr(*result.Context);

                if (!count.has_value() || count->getExtValue() >= 1)
                    return;

                const auto diagnostic = diag(value->getExprLoc(),
                                             "'getNumPrograms()' should return "
                                             "at least 1, as some hosts don't "
                                             "support plugins with 0 programs");

                if (isa<IntegerLiteral>(value->IgnoreParenImpCasts()))
                    diagnostic << FixItHint::CreateReplacement(value->getSourceRange(), "1");
            }
        };
    } // namespace program_count

    using program_count::ProgramCountCheck;
} // namespace clang::tidy::juce
