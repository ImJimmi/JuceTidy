#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace webview_event_before_load
    {
        class WebviewEventBeforeLoadCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("emitEventIfBrowserIsVisible",
                                                                      "evaluateJavascript"),
                                                           ofClass(hasName("::juce::WebBrowserComponent")))
                                                 .bind("method")),
                                      forCallable(cxxConstructorDecl()))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager)
                    || !isConstructedWithObject(*call->getImplicitObjectArgument()))
                {
                    return;
                }

                if (method->getName() == "emitEventIfBrowserIsVisible")
                {
                    diag(call->getExprLoc(),
                         "the page won't have loaded yet, so the frontend can't "
                         "receive this event. Use 'withInitialisationData()' to "
                         "pass initial state to the page");
                    return;
                }

                diag(call->getExprLoc(),
                     "the page won't have loaded yet, so this script won't run "
                     "in it. Use 'withUserScript()' to run scripts when the "
                     "page loads");
            }

        private:
            [[nodiscard]] static bool isConstructedWithObject(const Expr& browser)
            {
                if (isa<CXXThisExpr>(browser.IgnoreParenImpCasts()))
                    return true;

                const auto* field = utils::getReferencedField(browser);
                return field != nullptr && utils::isComponentOrOwnedComponent(field->getType());
            }
        };
    } // namespace webview_event_before_load

    using webview_event_before_load::WebviewEventBeforeLoadCheck;
} // namespace clang::tidy::juce
