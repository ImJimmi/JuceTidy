#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace listener_on_copy
    {
        class ListenerOnCopyCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto localCopy = declRefExpr(to(varDecl(hasLocalStorage(),
                                                              unless(hasType(referenceType())))));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("addListener"),
                                                           ofClass(cxxRecordDecl(hasAnyName("::juce::ValueTree",
                                                                                            "::juce::Value"))
                                                                       .bind("class")))),
                                      anyOf(on(localCopy),
                                            onImplicitObjectArgument(ignoringImpCasts(materializeTemporaryExpr()))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* listened = result.Nodes.getNodeAs<CXXRecordDecl>("class");
                const auto className = "juce::" + listened->getNameAsString();

                diag(call->getExprLoc(),
                     "listener added to a temporary '%0'. No callbacks will be "
                     "called")
                    << className;
            }
        };
    } // namespace listener_on_copy

    using listener_on_copy::ListenerOnCopyCheck;
} // namespace clang::tidy::juce
