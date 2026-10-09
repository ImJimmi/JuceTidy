#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace manual_deletion
    {
        class ManualDeletionCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    stmt(unless(isInTemplateInstantiation()),
                         anyOf(callExpr(callee(functionDecl(hasName("::juce::deleteAndZero"))))
                                   .bind("deleteAndZero"),
                               cxxMemberCallExpr(callee(cxxMethodDecl(hasName("::juce::Component::deleteAllChildren"))))
                                   .bind("deleteAllChildren"))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto& nodes = result.Nodes;

                if (const auto* call = nodes.getNodeAs<CallExpr>("deleteAndZero"))
                {
                    diag(call->getBeginLoc(),
                         "avoid 'juce::deleteAndZero()'. Prefer "
                         "std::unique_ptr, or some other automatic lifetime "
                         "management");
                }

                if (const auto* call = nodes.getNodeAs<CXXMemberCallExpr>("deleteAllChildren"))
                {
                    diag(call->getExprLoc(),
                         "avoid 'deleteAllChildren()'. Prefer managing "
                         "children with std::unique_ptr, or some other "
                         "automatic lifetime management");
                }
            }
        };
    } // namespace manual_deletion

    using manual_deletion::ManualDeletionCheck;
} // namespace clang::tidy::juce
