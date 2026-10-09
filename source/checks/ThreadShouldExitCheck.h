#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace thread_should_exit
    {
        class ThreadShouldExitCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const std::vector<std::string> run{ "juce::Thread::run" };

                finder->addMatcher(
                    stmt(unless(isInTemplateInstantiation()),
                         anyOf(whileStmt(),
                               doStmt(),
                               forStmt()),
                         forCallable(cxxMethodDecl(utils::isOrOverridesAnyOf(run),
                                                   unless(hasName("::juce::Thread::run")))
                                         .bind("run")))
                        .bind("loop"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* loop = result.Nodes.getNodeAs<Stmt>("loop");
                const auto* run = result.Nodes.getNodeAs<CXXMethodDecl>("run");
                auto& context = *result.Context;

                if (!isEndless(*loop, context) || loop->getBeginLoc().isMacroID())
                    return;

                const auto checksForExit = callExpr(callee(functionDecl(hasAnyName("threadShouldExit",
                                                                                   "currentThreadShouldExit"))));

                if (utils::findFirst(checksForExit, *loop, context) != nullptr)
                    return;

                diag(loop->getBeginLoc(),
                     "loop in '%0()' never checks 'threadShouldExit()' so the "
                     "thread has to be killed to be stopped")
                    << run->getName();
            }

        private:
            [[nodiscard]] static const Expr* getCondition(const Stmt& loop)
            {
                if (const auto* whileLoop = dyn_cast<WhileStmt>(&loop))
                    return whileLoop->getCond();

                if (const auto* doLoop = dyn_cast<DoStmt>(&loop))
                    return doLoop->getCond();

                return cast<ForStmt>(loop).getCond();
            }

            [[nodiscard]] static bool isEndless(const Stmt& loop, const ASTContext& context)
            {
                const auto* condition = getCondition(loop);

                if (condition == nullptr)
                    return true;

                bool value = false;

                return !condition->isValueDependent()
                    && condition->EvaluateAsBooleanCondition(value, context)
                    && value;
            }
        };
    } // namespace thread_should_exit

    using thread_should_exit::ThreadShouldExitCheck;
} // namespace clang::tidy::juce
