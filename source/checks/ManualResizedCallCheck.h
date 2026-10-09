#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace manual_resized_call
    {
        class ManualResizedCallCheck : public JuceTidyCheck
        {
        public:
            ManualResizedCallCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , layoutFunctionName{ Options.get("LayoutFunctionName", "updateLayout") }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "LayoutFunctionName", layoutFunctionName);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("resized"),
                                                           parameterCountIs(0),
                                                           ofClass(isSameOrDerivedFrom(hasName("::juce::Component"))))),
                                      forCallable(functionDecl().bind("caller")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* caller = result.Nodes.getNodeAs<FunctionDecl>("caller");

                if (isBaseClassLayoutCall(*call, *caller, layoutFunctionName)
                    || utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                {
                    return;
                }

                const auto diagnostic = diag(call->getExprLoc(),
                                             "`resized()` is a callback, don't "
                                             "call it directly. Move the "
                                             "layout code to '%0()', call "
                                             "'%0()' from 'resized()', and "
                                             "call '%0()' here")
                                     << layoutFunctionName;

                const auto* member = dyn_cast<MemberExpr>(call->getCallee()->IgnoreParens());

                if (member == nullptr
                    || member->hasQualifier()
                    || member->getMemberLoc().isMacroID()
                    || !utils::hasMethodNamed(call->getRecordDecl(), layoutFunctionName))
                {
                    return;
                }

                diagnostic << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(member->getMemberLoc()), layoutFunctionName);
            }

        private:
            [[nodiscard]] static bool isBaseClassLayoutCall(const CXXMemberCallExpr& call,
                                                            const FunctionDecl& caller,
                                                            StringRef layoutFunctionName)
            {
                const auto* member = dyn_cast<MemberExpr>(call.getCallee()->IgnoreParens());

                return member != nullptr
                    && member->hasQualifier()
                    && caller.getDeclName().isIdentifier()
                    && (caller.getName() == "resized" || caller.getName() == layoutFunctionName);
            }

            const StringRef layoutFunctionName;
        };
    } // namespace manual_resized_call

    using manual_resized_call::ManualResizedCallCheck;
} // namespace clang::tidy::juce
