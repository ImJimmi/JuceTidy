#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unowned_child_component
    {
        class UnownedChildComponentCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("addAndMakeVisible",
                                                                      "addChildComponent",
                                                                      "addChildAndSetID",
                                                                      "setContentNonOwned"),
                                                           ofClass(isSameOrDerivedFrom(hasName("::juce::Component"))))
                                                 .bind("method")),
                                      hasArgument(0, ignoringParenImpCasts(cxxNewExpr().bind("allocation")))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                const auto* allocation = result.Nodes.getNodeAs<CXXNewExpr>("allocation");

                diag(allocation->getBeginLoc(),
                     "allocating a new component but '%0()' doesn't take "
                     "ownership of it")
                    << method->getName();
            }
        };
    } // namespace unowned_child_component

    using unowned_child_component::UnownedChildComponentCheck;
} // namespace clang::tidy::juce
