#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <algorithm>
#include <array>
#include <ranges>

namespace clang::tidy::juce
{
    namespace webview_windows_options
    {
        class WebviewWindowsOptionsCheck : public JuceTidyCheck
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
                const auto* options = utils::getWebBrowserOptionsArgument(*result.Nodes.getNodeAs<Expr>("browser"));

                if (options == nullptr)
                    return;

                auto& context = *result.Context;
                auto [calls, isComplete] = utils::findWebBrowserOptionsCalls(*options, context);

                if (!isComplete)
                    return;

                llvm::sort(calls, [&context](const CXXMemberCallExpr* a, const CXXMemberCallExpr* b) {
                    return context.getSourceManager().isBeforeInTranslationUnit(a->getExprLoc(), b->getExprLoc());
                });

                const auto latestFirst = calls | std::views::reverse;
                const auto lastBackend = std::ranges::find_if(latestFirst, [](const CXXMemberCallExpr* call) {
                    return utils::getMethodName(*call) == "withBackend";
                });
                const auto* backend = lastBackend != latestFirst.end() ? *lastBackend : nullptr;
                const auto backendName = backend != nullptr && backend->getNumArgs() == 1
                                           ? getEnumeratorName(*backend->getArg(0), context)
                                           : std::optional<StringRef>{};

                if (backend != nullptr && !backendName)
                    return;

                if (backendName != "webview2")
                {
                    const auto feature = llvm::find_if(calls, [&context](const CXXMemberCallExpr* call) {
                        return needsWebView2(*call, context);
                    });

                    if (feature != calls.end())
                    {
                        diag((*feature)->getExprLoc(),
                             "'%0()' only works on Windows with the WebView2 "
                             "backend. Add '.withBackend(juce::WebBrowserComponent::"
                             "Options::Backend::webview2)'")
                            << utils::getMethodName(**feature);
                    }

                    return;
                }

                const auto setsUserDataFolder = llvm::any_of(calls, [](const CXXMemberCallExpr* call) {
                    return utils::getMethodName(*call) == "withUserDataFolder";
                });

                if (setsUserDataFolder)
                    return;

                diag(backend->getExprLoc(),
                     "WebView2 needs a user data folder it can write to, which "
                     "the default next to the executable often isn't. Set one "
                     "with 'WinWebView2::withUserDataFolder()'");
            }

        private:
            static inline constexpr std::array webView2Features{
                StringRef{ "withNativeFunction" },
                StringRef{ "withEventListener" },
                StringRef{ "withUserScript" },
                StringRef{ "withInitialisationData" },
                StringRef{ "withResourceProvider" },
                StringRef{ "withOptionsFrom" },
            };

            [[nodiscard]] static bool needsWebView2(const CXXMemberCallExpr& call, const ASTContext& context)
            {
                const auto name = utils::getMethodName(call);

                if (name == "withNativeIntegrationEnabled")
                {
                    auto enabled = false;
                    return call.getNumArgs() == 1
                        && call.getArg(0)->EvaluateAsBooleanCondition(enabled, context)
                        && enabled;
                }

                return llvm::is_contained(webView2Features, name);
            }

            [[nodiscard]] static std::optional<StringRef> getEnumeratorName(const Expr& value, const ASTContext& context)
            {
                const auto* enumeration = dyn_cast_or_null<EnumDecl>(value.getType()->getAsTagDecl());
                Expr::EvalResult result;

                if (enumeration == nullptr || !value.EvaluateAsInt(result, context))
                    return std::nullopt;

                for (const auto* enumerator : enumeration->enumerators())
                {
                    if (llvm::APSInt::isSameValue(enumerator->getInitVal(), result.Val.getInt()))
                        return enumerator->getName();
                }

                return std::nullopt;
            }
        };
    } // namespace webview_windows_options

    using webview_windows_options::WebviewWindowsOptionsCheck;
} // namespace clang::tidy::juce
