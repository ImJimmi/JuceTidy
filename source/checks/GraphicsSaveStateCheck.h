#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace graphics_save_state
    {
        class GraphicsSaveStateCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(functionDecl(isDefinition(),
                                                unless(isInstantiated()),
                                                hasBody(hasDescendant(graphicsCall("saveState"))))
                                       .bind("function"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                auto& context = *result.Context;
                const auto& sourceManager = *result.SourceManager;
                const auto inThisFunction = forCallable(equalsNode(static_cast<const Decl*>(function)));

                const auto locations = [&](const ast_matchers::internal::Matcher<Stmt>& matcher) {
                    std::vector<SourceLocation> found;

                    for (const auto& nodes : match(findAll(stmt(matcher, inThisFunction).bind("node")),
                                                   *function->getBody(),
                                                   context))
                    {
                        found.push_back(nodes.getNodeAs<Stmt>("node")->getBeginLoc());
                    }

                    llvm::sort(found,
                               [&sourceManager](SourceLocation a, SourceLocation b) {
                                   return sourceManager.isBeforeInTranslationUnit(a, b);
                               });
                    return found;
                };

                const auto saves = locations(graphicsCall("saveState"));
                const auto restores = locations(graphicsCall("restoreState"));

                if (saves.empty())
                    return;

                if (restores.size() < saves.size())
                {
                    diag(saves.front(),
                         "'saveState()' called without a matching "
                         "'restoreState()'. Use "
                         "'juce::Graphics::ScopedSaveState' instead");

                    return;
                }

                for (const auto location : locations(returnStmt()))
                {
                    if (sourceManager.isBeforeInTranslationUnit(saves.front(), location)
                        && sourceManager.isBeforeInTranslationUnit(location, restores.back()))
                    {
                        diag(location,
                             "returning early before 'restoreState' is called. "
                             "Use 'juce::Graphics::ScopedSaveState' instead");

                        return;
                    }
                }
            }

        private:
            [[nodiscard]] static ast_matchers::internal::Matcher<Stmt> graphicsCall(StringRef name)
            {
                using namespace clang::ast_matchers;

                return cxxMemberCallExpr(callee(cxxMethodDecl(hasName(name),
                                                              ofClass(hasName("::juce::Graphics")))))
                    .bind("call");
            }
        };
    } // namespace graphics_save_state

    using graphics_save_state::GraphicsSaveStateCheck;
} // namespace clang::tidy::juce
