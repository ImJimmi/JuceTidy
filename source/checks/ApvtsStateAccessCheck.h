#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace apvts_state_access
    {
        class ApvtsStateAccessCheck final : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto state = memberExpr(member(fieldDecl(hasName("state"),
                                                               hasDeclContext(cxxRecordDecl(hasName("::juce::AudioProcessorValueTreeState"))))))
                                       .bind("state");

                const auto inMethod = [](StringRef name) {
                    return forCallable(cxxMethodDecl(utils::isOrOverridesAnyOf({ name.str() })).bind("method"));
                };

                finder->addMatcher(memberExpr(state,
                                              unless(isInTemplateInstantiation()),
                                              inMethod("juce::AudioProcessor::getStateInformation")),
                                   this);

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(cxxOperatorCallExpr(hasOverloadedOperatorName("="),
                                                   hasArgument(0, ignoringParenImpCasts(state))),
                               cxxMemberCallExpr(on(state),
                                                 callee(cxxMethodDecl(unless(isConst()))))),
                         inMethod("juce::AudioProcessor::setStateInformation")),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* state = result.Nodes.getNodeAs<MemberExpr>("state");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");

                if (utils::isInMacroBody(state->getExprLoc(), *result.SourceManager))
                    return;

                if (method->getName() == "getStateInformation")
                {
                    diag(state->getExprLoc(),
                         "reading 'state' directly in 'getStateInformation()' "
                         "isn't thread-safe. Use 'copyState()' instead");
                }
                else
                {
                    diag(state->getExprLoc(),
                         "changing 'state' directly in 'setStateInformation()' "
                         "isn't thread-safe. Use 'replaceState(newState)' "
                         "instead");
                }
            }
        };
    } // namespace apvts_state_access

    using apvts_state_access::ApvtsStateAccessCheck;
} // namespace clang::tidy::juce
