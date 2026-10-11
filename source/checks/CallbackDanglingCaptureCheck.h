#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace callback_dangling_capture
    {
        class CallbackDanglingCaptureCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto callback = memberExpr(member(fieldDecl(matchesName("^::juce::.*::on[A-Z][A-Za-z]*$"),
                                                                  hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(hasName("::std::function")))))))
                                                            .bind("callback")));
                const auto lambda = lambdaExpr().bind("lambda");

                const auto convertedLambda = ignoringImplicit(anyOf(lambda,
                                                                    cxxConstructExpr(hasArgument(0, ignoringImplicit(lambda)))));

                finder->addMatcher(
                    cxxOperatorCallExpr(unless(isInTemplateInstantiation()),
                                        hasOverloadedOperatorName("="),
                                        hasArgument(0, ignoringParenImpCasts(callback)),
                                        hasArgument(1, convertedLambda))
                        .bind("assignment"),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(anyOf(allOf(hasAnyName("withNativeFunction",
                                                                                  "withEventListener",
                                                                                  "withResourceProvider"),
                                                                       ofClass(hasName("::juce::WebBrowserComponent::Options"))),
                                                                 allOf(hasName("evaluateJavascript"),
                                                                       ofClass(hasName("::juce::WebBrowserComponent")))))
                                                 .bind("registration")),
                                      hasAnyArgument(convertedLambda)),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* callback = result.Nodes.getNodeAs<FieldDecl>("callback");
                const auto* registration = result.Nodes.getNodeAs<CXXMethodDecl>("registration");
                const auto* lambda = result.Nodes.getNodeAs<LambdaExpr>("lambda");

                for (const auto& capture : lambda->captures())
                {
                    if (!capture.capturesVariable() || capture.getCaptureKind() != LCK_ByRef)
                        continue;

                    const auto* variable = dyn_cast<VarDecl>(capture.getCapturedVar());

                    if (variable == nullptr
                        || !variable->hasLocalStorage()
                        || variable->getType()->isReferenceType())
                    {
                        continue;
                    }

                    if (callback != nullptr)
                    {
                        diag(capture.getLocation(),
                             "%0 is a local variable captured by reference which "
                             "won't exist when %1 is called. Capture by value "
                             "instead")
                            << variable
                            << callback;
                        continue;
                    }

                    diag(capture.getLocation(),
                         "%0 is a local variable captured by reference which "
                         "won't exist when the callback passed to %1 is called. "
                         "Capture by value instead")
                        << variable
                        << registration;
                }
            }
        };
    } // namespace callback_dangling_capture

    using callback_dangling_capture::CallbackDanglingCaptureCheck;
} // namespace clang::tidy::juce
