#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace opengl_detach
    {
        class OpenGlDetachCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("attachTo"),
                                                           ofClass(hasName("::juce::OpenGLContext")))),
                                      on(memberExpr(member(fieldDecl().bind("context")))),
                                      forCallable(cxxMethodDecl(ofClass(cxxRecordDecl(unless(isLambda())).bind("owner")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* context = result.Nodes.getNodeAs<FieldDecl>("context");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");

                const auto detach = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("detach"),
                                                                           ofClass(hasName("::juce::OpenGLContext")))),
                                                      on(memberExpr(member(equalsNode(context)))));

                if (utils::searchDestructor(*owner, detach, *result.Context) != utils::DestructorSearch::notFound)
                    return;

                diag(call->getExprLoc(),
                     "'%0' is attached here but never detached. Call "
                     "'%0.detach()' somewhere before the context or its "
                     "component are destroyed")
                    << context->getName();
            }
        };
    } // namespace opengl_detach

    using opengl_detach::OpenGlDetachCheck;
} // namespace clang::tidy::juce
