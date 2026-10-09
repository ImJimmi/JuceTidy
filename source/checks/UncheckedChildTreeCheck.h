#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unchecked_child_tree
    {
        class UncheckedChildTreeCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto mutation = cxxMethodDecl(hasAnyName("setProperty",
                                                               "setPropertyExcludingListener",
                                                               "removeProperty",
                                                               "removeAllProperties",
                                                               "appendChild",
                                                               "addChild",
                                                               "removeChild",
                                                               "removeAllChildren",
                                                               "moveChild",
                                                               "copyPropertiesFrom",
                                                               "copyPropertiesAndChildrenFrom"),
                                                    ofClass(hasName("::juce::ValueTree")))
                                          .bind("mutation");

                finder->addMatcher(cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                                     callee(mutation),
                                                     on(lookup()))
                                       .bind("call"),
                                   this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(mutation),
                                      on(declRefExpr(to(varDecl(hasLocalStorage(),
                                                                hasInitializer(ignoringImplicit(lookup())))
                                                            .bind("variable")))),
                                      forCallable(functionDecl().bind("function")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* lookupMethod = result.Nodes.getNodeAs<CXXMethodDecl>("lookupMethod");
                const auto* mutation = result.Nodes.getNodeAs<CXXMethodDecl>("mutation");
                const auto* variable = result.Nodes.getNodeAs<VarDecl>("variable");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                const auto* const suggestion = lookupMethod->getName() == "getChildWithName"
                                                 ? " or use 'getOrCreateChildWithName()'"
                                                 : "";

                if (variable == nullptr)
                {
                    diag(call->getExprLoc(),
                         "'%0()' might be called on an invalid tree, as '%1()' "
                         "can return invalid trees. Check 'isValid()' first%2")
                        << mutation->getName()
                        << lookupMethod->getName()
                        << suggestion;

                    return;
                }

                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                const auto validityCheck = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("isValid",
                                                                                             "hasType"))),
                                                             on(declRefExpr(to(equalsNode(variable)))));

                if (function->getBody() != nullptr
                    && utils::findFirst(validityCheck, *function->getBody(), *result.Context) != nullptr)
                {
                    return;
                }

                diag(call->getExprLoc(),
                     "%0 may be an invalid tree, as `%1()` returns an invalid "
                     "tree when there's nothing to return. Check "
                     "'%3.isValid()' first%4")
                    << variable
                    << lookupMethod->getName()
                    << mutation
                    << variable->getName()
                    << suggestion;
            }

        private:
            [[nodiscard]] static ast_matchers::internal::Matcher<Stmt> lookup()
            {
                using namespace clang::ast_matchers;

                return cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("getChildWithName",
                                                                         "getChildWithProperty",
                                                                         "getChild",
                                                                         "getParent",
                                                                         "getSibling"),
                                                              ofClass(hasName("::juce::ValueTree")))
                                                    .bind("lookupMethod")))
                    .bind("lookup");
            }
        };
    } // namespace unchecked_child_tree

    using unchecked_child_tree::UncheckedChildTreeCheck;
} // namespace clang::tidy::juce
