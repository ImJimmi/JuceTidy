#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unrestricted_webview_navigation
    {
        class UnrestrictedWebviewNavigationCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(expr(unless(isInTemplateInstantiation()),
                                        utils::webBrowserConstruction())
                                       .bind("browser"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* construction = result.Nodes.getNodeAs<Expr>("browser");
                const auto* options = utils::getWebBrowserOptionsArgument(*construction);
                auto& context = *result.Context;

                if (options == nullptr)
                    return;

                const auto* browser = getConstructedClass(*construction, context);

                if (browser == nullptr
                    || overridesPageAboutToLoad(browser)
                    || !enablesNativeIntegration(utils::findWebBrowserOptionsCalls(*options, context).calls, context))
                {
                    return;
                }

                const auto name = utils::qualifiedName(*browser);
                const auto isDerived = name != "juce::WebBrowserComponent";

                diag(construction->getBeginLoc(),
                     "%select{don't use a non-derived 'juce::WebBrowserComponent' "
                     "as its default implementation of 'pageAboutToLoad()'|"
                     "'%1' doesn't override 'pageAboutToLoad()'. The default "
                     "implementation}0 "
                     "allows the browser to navigate to any webpage, which "
                     "could then access the C++ backend. "
                     "%select{Write a derived class and override|Override}0 "
                     "'pageAboutToLoad()' to restrict which URLs can be "
                     "accessed by this browser")
                    << isDerived
                    << name;
            }

        private:
            [[nodiscard]] static const CXXRecordDecl* getConstructedClass(const Expr& construction, ASTContext& context)
            {
                if (const auto* call = dyn_cast<CallExpr>(&construction))
                {
                    const auto* function = call->getDirectCallee();
                    const auto* arguments = function != nullptr
                                              ? function->getTemplateSpecializationArgs()
                                              : nullptr;

                    return arguments != nullptr && arguments->size() > 0
                             ? arguments->get(0).getAsType()->getAsCXXRecordDecl()
                             : nullptr;
                }

                auto node = DynTypedNode::create(construction);

                while (true)
                {
                    const auto parents = context.getParents(node);

                    if (parents.size() != 1)
                        break;

                    if (const auto* constructor = parents.begin()->get<CXXConstructorDecl>())
                    {
                        const auto* initialised = node.get<Expr>();
                        const auto initialisesBase = llvm::any_of(constructor->inits(),
                                                                  [initialised](const CXXCtorInitializer* initialiser) {
                                                                      return initialiser->isBaseInitializer() && initialiser->getInit() == initialised;
                                                                  });

                        if (initialisesBase)
                            return constructor->getParent();

                        break;
                    }

                    if (!isa_and_nonnull<ExprWithCleanups>(parents.begin()->get<Expr>()))
                        break;

                    node = *parents.begin();
                }

                return construction.getType()->getAsCXXRecordDecl();
            }

            [[nodiscard]] static bool overridesPageAboutToLoad(const CXXRecordDecl* record)
            {
                const auto* definition = record != nullptr ? record->getDefinition() : nullptr;

                if (definition == nullptr || utils::qualifiedName(*definition) == "juce::WebBrowserComponent")
                    return false;

                const auto overrides = llvm::any_of(definition->methods(),
                                                    [](const CXXMethodDecl* method) {
                                                        return method->getDeclName().isIdentifier()
                                                            && method->getName() == "pageAboutToLoad"
                                                            && method->size_overridden_methods() > 0;
                                                    });

                return overrides
                    || llvm::any_of(definition->bases(),
                                    [](const CXXBaseSpecifier& base) {
                                        return overridesPageAboutToLoad(utils::getRecord(base.getType()));
                                    });
            }

            [[nodiscard]] static bool enablesNativeIntegration(ArrayRef<const CXXMemberCallExpr*> calls,
                                                               const ASTContext& context)
            {
                auto enabled = false;

                for (const auto* call : calls)
                {
                    if (utils::getMethodName(*call) != "withNativeIntegrationEnabled")
                        continue;

                    auto value = false;

                    if (call->getNumArgs() != 1
                        || !call->getArg(0)->EvaluateAsBooleanCondition(value, context)
                        || !value)
                    {
                        return false;
                    }

                    enabled = true;
                }

                return enabled;
            }
        };
    } // namespace unrestricted_webview_navigation

    using unrestricted_webview_navigation::UnrestrictedWebviewNavigationCheck;
} // namespace clang::tidy::juce
