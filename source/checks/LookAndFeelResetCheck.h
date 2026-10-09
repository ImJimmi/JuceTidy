#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace look_and_feel_reset
    {
        class LookAndFeelResetCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(lookAndFeelSetter().bind("setter")),
                             hasArgument(0, unless(nullPointer())),
                             forCallable(cxxMethodDecl(unless(cxxDestructorDecl()),
                                                       ofClass(cxxRecordDecl(unless(isLambda())).bind("owner")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<CallExpr>("call");
                const auto* setter = result.Nodes.getNodeAs<FunctionDecl>("setter");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");
                auto& context = *result.Context;
                const auto* target = getTarget(*call);

                if (isOutlivedByLookAndFeel(*call, target))
                    return;

                const auto reset = callExpr(callee(functionDecl(hasName(setter->getName()))),
                                            hasArgument(0, nullPointer()))
                                       .bind("reset");
                const auto* destructor = owner->getDestructor();
                const auto* body = destructor != nullptr
                                        && destructor->isUserProvided()
                                        && destructor->getDefinition() != nullptr
                                     ? destructor->getDefinition()->getBody()
                                     : nullptr;

                if (destructor != nullptr
                    && destructor->isUserProvided()
                    && body == nullptr)
                {
                    return;
                }

                if (body != nullptr)
                {
                    const auto resets = match(findAll(reset), *body, context);

                    if (llvm::any_of(resets,
                                     [target](const BoundNodes& nodes) {
                                         return getTarget(*nodes.getNodeAs<CallExpr>("reset")) == target;
                                     }))
                    {
                        return;
                    }
                }

                diag(call->getBeginLoc(),
                     "'%0' is called here but '%0(nullptr)' is never called so "
                     "it will still be in use when it's destroyed.")
                    << setter->getName()
                    << owner;
            }

        private:
            [[nodiscard]] static ast_matchers::internal::Matcher<Expr> nullPointer()
            {
                using namespace clang::ast_matchers;

                return ignoringParenImpCasts(anyOf(cxxNullPtrLiteralExpr(),
                                                   gnuNullExpr(),
                                                   integerLiteral(equals(0))));
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Decl> lookAndFeelSetter()
            {
                using namespace clang::ast_matchers;

                return functionDecl(hasAnyName("::juce::Component::setLookAndFeel",
                                               "::juce::LookAndFeel::setDefaultLookAndFeel"));
            }

            [[nodiscard]] static const Decl* getTarget(const CallExpr& call)
            {
                const auto* memberCall = dyn_cast<CXXMemberCallExpr>(&call);

                if (memberCall == nullptr)
                    return nullptr;

                const auto* object = memberCall->getImplicitObjectArgument()->IgnoreParenImpCasts();

                if (const auto* member = dyn_cast<MemberExpr>(object))
                    return member->getMemberDecl();

                if (const auto* reference = dyn_cast<DeclRefExpr>(object))
                    return reference->getDecl();

                return nullptr;
            }

            [[nodiscard]] static bool isOutlivedByLookAndFeel(const CallExpr& call, const Decl* target)
            {
                const auto* child = dyn_cast_or_null<FieldDecl>(target);
                const auto* lookAndFeel = call.getNumArgs() > 0
                                            ? utils::getReferencedField(*call.getArg(0))
                                            : nullptr;

                return child != nullptr
                    && lookAndFeel != nullptr
                    && child->getParent() == lookAndFeel->getParent()
                    && lookAndFeel->getFieldIndex() < child->getFieldIndex();
            }
        };
    } // namespace look_and_feel_reset

    using look_and_feel_reset::LookAndFeelResetCheck;
} // namespace clang::tidy::juce
