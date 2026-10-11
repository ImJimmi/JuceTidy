#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace native_function_completion
    {
        class NativeFunctionCompletionCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto lambda = lambdaExpr().bind("lambda");

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("withNativeFunction"),
                                                           ofClass(hasName("::juce::WebBrowserComponent::Options")))),
                                      hasArgument(1,
                                                  ignoringImplicit(anyOf(lambda,
                                                                         cxxConstructExpr(hasArgument(0, ignoringImplicit(lambda))))))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* callOperator = result.Nodes.getNodeAs<LambdaExpr>("lambda")->getCallOperator();

                if (callOperator->getNumParams() != 2)
                    return;

                const auto* completion = callOperator->getParamDecl(1);

                if (completion->isParameterPack() || completion->isReferenced())
                    return;

                if (completion->getIdentifier() == nullptr)
                {
                    diag(completion->getLocation(),
                         "the NativeFunctionCompletion is never called, so the "
                         "promise returned to JavaScript never resolves");
                    return;
                }

                diag(completion->getLocation(),
                     "the NativeFunctionCompletion, %0, is never called, so "
                     "the promise returned to JavaScript never resolves")
                    << completion;
            }
        };
    } // namespace native_function_completion

    using native_function_completion::NativeFunctionCompletionCheck;
} // namespace clang::tidy::juce
