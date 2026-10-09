#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace async_this_capture
    {
        class AsyncThisCaptureCheck : public JuceTidyCheck
        {
        public:
            AsyncThisCaptureCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawAsyncFunctions{ Options.get("AsyncFunctions", defaultAsyncFunctions) }
                , asyncFunctions{ utils::parseQualifiedNames(rawAsyncFunctions) }
                , rawSafePointerTypes{ Options.get("SafePointerTypes", defaultSafePointerTypes) }
                , safePointerTypes{ utils::parseQualifiedNames(rawSafePointerTypes) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "AsyncFunctions", rawAsyncFunctions);
                Options.store(options, "SafePointerTypes", rawSafePointerTypes);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto lambda = lambdaExpr().bind("lambda");

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(utils::isOrOverridesAnyOf(asyncFunctions))
                                        .bind("function")),
                             hasAnyArgument(ignoringImplicit(anyOf(lambda,
                                                                   cxxConstructExpr(hasArgument(0, ignoringImplicit(lambda))))))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                const auto* lambda = result.Nodes.getNodeAs<LambdaExpr>("lambda");

                const auto isSafePointer = [this](const LambdaCapture& capture) {
                    return capture.capturesVariable()
                        && utils::isSameOrDerivedFrom(capture
                                                          .getCapturedVar()
                                                          ->getType()
                                                          .getNonReferenceType()
                                                          ->getAsCXXRecordDecl(),
                                                      safePointerTypes);
                };

                if (llvm::any_of(lambda->captures(), isSafePointer))
                    return;

                const auto* thisCapture = llvm::find_if(lambda->captures(),
                                                        [](const LambdaCapture& capture) {
                                                            return capture.capturesThis() && capture.getCaptureKind() == LCK_This;
                                                        });

                if (thisCapture == lambda->capture_end())
                    return;

                diag(thisCapture->getLocation(),
                     "'this' may be dangling by the time the callback is "
                     "called. Use a juce::Component::SafePointer or "
                     "juce::WeakReference instead")
                    << function;
            }

        private:
            static inline constexpr auto defaultAsyncFunctions =
                "::juce::MessageManager::callAsync;"
                "::juce::Timer::callAfterDelay;"
                "::juce::Thread::launch;"
                "::juce::NativeMessageBox::showAsync;"
                "::juce::AlertWindow::showAsync";
            static inline constexpr auto defaultSafePointerTypes =
                "::juce::Component::SafePointer;"
                "::juce::WeakReference;"
                "::std::weak_ptr";

            const StringRef rawAsyncFunctions;
            const std::vector<std::string> asyncFunctions;
            const StringRef rawSafePointerTypes;
            const std::vector<std::string> safePointerTypes;
        };
    } // namespace async_this_capture

    using async_this_capture::AsyncThisCaptureCheck;
} // namespace clang::tidy::juce
