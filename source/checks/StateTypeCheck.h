#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace state_type_check
    {
        class StateTypeCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const std::vector<std::string> setState{ "juce::AudioProcessor::setStateInformation" };
                const auto apvts = cxxRecordDecl(hasName("::juce::AudioProcessorValueTreeState"));
                const auto replaceState = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("replaceState"),
                                                                                 ofClass(apvts))));
                const auto assignState = cxxOperatorCallExpr(isAssignmentOperator(),
                                                             hasArgument(0,
                                                                         memberExpr(member(fieldDecl(hasName("state"),
                                                                                                     hasDeclContext(apvts))))));

                finder->addMatcher(expr(unless(isInTemplateInstantiation()),
                                        anyOf(replaceState,
                                              assignState),
                                        forCallable(cxxMethodDecl(utils::isOrOverridesAnyOf(setState),
                                                                  hasBody(stmt().bind("body")))))
                                       .bind("call"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<Expr>("call");
                const auto* body = result.Nodes.getNodeAs<Stmt>("body");
                const auto typeCheck = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("hasTagName",
                                                                                         "hasType",
                                                                                         "getType",
                                                                                         "getTagName",
                                                                                         "getChildWithName"))));

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager)
                    || utils::findFirst(typeCheck, *body, *result.Context) != nullptr)
                {
                    return;
                }

                diag(call->getExprLoc(),
                     "'setStateInformation()' replaces state without checking "
                     "the type of the new state. Check "
                     "'xml->hasTagName(apvts.state.getType())' first");
            }
        };
    } // namespace state_type_check

    using state_type_check::StateTypeCheck;
} // namespace clang::tidy::juce
