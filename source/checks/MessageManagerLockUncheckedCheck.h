#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace message_manager_lock_unchecked
    {
        class MessageManagerLockUncheckedCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto watchedThread = expr(unless(cxxDefaultArgExpr()),
                                                unless(ignoringParenImpCasts(anyOf(cxxNullPtrLiteralExpr(),
                                                                                   gnuNullExpr(),
                                                                                   integerLiteral(equals(0))))));

                finder->addMatcher(
                    varDecl(unless(isInstantiated()),
                            hasType(cxxRecordDecl(hasName("::juce::MessageManagerLock"))),
                            hasInitializer(ignoringImplicit(cxxConstructExpr(argumentCountIs(1),
                                                                             hasArgument(0, watchedThread)))),
                            hasAncestor(functionDecl(hasBody(stmt().bind("body")))))
                        .bind("lock"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* lock = result.Nodes.getNodeAs<VarDecl>("lock");
                const auto* body = result.Nodes.getNodeAs<Stmt>("body");

                const auto checksLock = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("lockWasGained"))),
                                                          on(declRefExpr(to(equalsNode(lock)))));

                if (lock->getLocation().isMacroID()
                    || utils::findFirst(checksLock, *body, *result.Context) != nullptr)
                {
                    return;
                }

                diag(lock->getLocation(),
                     "MessageManagerLock() might return without acquiring the "
                     "lock if the thread is given the signal to stop in the "
                     "meantime. Check '%0.lockWasGained()' to handle this case")
                    << lock->getName();
            }
        };
    } // namespace message_manager_lock_unchecked

    using message_manager_lock_unchecked::MessageManagerLockUncheckedCheck;
} // namespace clang::tidy::juce
