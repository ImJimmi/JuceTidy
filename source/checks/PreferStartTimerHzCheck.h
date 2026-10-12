#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>

namespace clang::tidy::juce
{
    namespace prefer_start_timer_hz
    {
        class PreferStartTimerHzCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("startTimer"),
                                                           ofClass(hasName("::juce::Timer")))),
                                      hasArgument(0, ignoringParenImpCasts(binaryOperator(hasOperatorName("/"), hasLHS(ignoringParenImpCasts(integerLiteral(equals(1000)))), hasRHS(expr().bind("frequency"))))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* frequency = result.Nodes.getNodeAs<Expr>("frequency");

                if (call->getExprLoc().isMacroID())
                    return;

                const auto frequencyText = utils::getSourceText(frequency->getSourceRange(), *result.SourceManager, getLangOpts());
                const auto wholeCall = utils::getFileRange({ call->getExprLoc(), call->getRParenLoc() }, *result.SourceManager, getLangOpts());

                if (!frequencyText.has_value() || !wholeCall.has_value())
                    return;

                diag(call->getExprLoc(),
                     "use 'startTimerHz(%0)' which is more accurate than the "
                     "integer division used here")
                    << *frequencyText
                    << FixItHint::CreateReplacement(*wholeCall, ("startTimerHz(" + *frequencyText + ")").str());
            }
        };
    } // namespace prefer_start_timer_hz

    using prefer_start_timer_hz::PreferStartTimerHzCheck;
} // namespace clang::tidy::juce
