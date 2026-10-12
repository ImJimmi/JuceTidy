#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace valuetree_listener_reentrancy
    {
        class ValuetreeListenerReentrancyCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto valueTree = cxxRecordDecl(hasName("::juce::ValueTree"));
                const auto callback = cxxMethodDecl(unless(isInstantiated()),
                                                    utils::isOrOverridesAnyOf({
                                                        "juce::ValueTree::Listener::valueTreePropertyChanged",
                                                        "juce::ValueTree::Listener::valueTreeChildAdded",
                                                        "juce::ValueTree::Listener::valueTreeChildRemoved",
                                                        "juce::ValueTree::Listener::valueTreeChildOrderChanged",
                                                        "juce::ValueTree::Listener::valueTreeParentChanged",
                                                        "juce::ValueTree::Listener::valueTreeRedirected",
                                                    }))
                                          .bind("callback");

                finder->addMatcher(
                    cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("setProperty",
                                                                      "removeProperty",
                                                                      "removeAllProperties",
                                                                      "appendChild",
                                                                      "addChild",
                                                                      "removeChild",
                                                                      "removeAllChildren",
                                                                      "moveChild",
                                                                      "getOrCreateChildWithName",
                                                                      "copyPropertiesFrom",
                                                                      "copyPropertiesAndChildrenFrom"),
                                                           ofClass(valueTree))),
                                      on(expr().bind("tree")),
                                      forCallable(callback))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* tree = result.Nodes.getNodeAs<Expr>("tree");
                const auto* callback = result.Nodes.getNodeAs<CXXMethodDecl>("callback");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                const auto* parameter = getRootParameter(*tree);

                if (parameter == nullptr || !llvm::is_contained(callback->parameters(), parameter))
                    return;

                diag(call->getExprLoc(),
                     "changing '%0' inside '%1()' causes a synchronous "
                     "listener callback meaning any listeners that haven't "
                     "been notified of the previous change yet will be "
                     "notified of this later change first. Make the change "
                     "asynchronously instead, for example with "
                     "juce::MessageManager::callAsync() or juce::AsyncUpdater, "
                     "so all listeners respond to the changes in the correct "
                     "order")
                    << parameter->getName()
                    << callback->getName();
            }

        private:
            [[nodiscard]] static const ParmVarDecl* getRootParameter(const Expr& tree)
            {
                const auto* current = tree.IgnoreParenImpCasts();

                while (true)
                {
                    if (const auto* construction = dyn_cast<CXXConstructExpr>(current); construction != nullptr && construction->getNumArgs() == 1)
                    {
                        current = construction->getArg(0)->IgnoreParenImpCasts();
                        continue;
                    }

                    if (const auto* call = dyn_cast<CXXMemberCallExpr>(current))
                    {
                        const auto name = utils::getMethodName(*call);

                        if (name.starts_with("getParent") || name.starts_with("getChild") || name == "getRoot" || name == "getSibling" || name == "getOrCreateChildWithName")
                        {
                            current = call->getImplicitObjectArgument()->IgnoreParenImpCasts();
                            continue;
                        }

                        return nullptr;
                    }

                    if (const auto* reference = dyn_cast<DeclRefExpr>(current))
                        return dyn_cast<ParmVarDecl>(reference->getDecl());

                    return nullptr;
                }
            }
        };
    } // namespace valuetree_listener_reentrancy

    using valuetree_listener_reentrancy::ValuetreeListenerReentrancyCheck;
} // namespace clang::tidy::juce
