#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace empty_state_information
    {
        class EmptyStateInformationCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMethodDecl(isDefinition(),
                                  unless(isInstantiated()),
                                  utils::isOrOverridesAnyOf({
                                      "juce::AudioProcessor::getStateInformation",
                                      "juce::AudioProcessor::setStateInformation",
                                  }),
                                  hasBody(compoundStmt().bind("body")))
                        .bind("method"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                const auto* body = result.Nodes.getNodeAs<CompoundStmt>("body");

                if (method->getLocation().isMacroID() || !llvm::all_of(body->body(), isPlaceholder))
                    return;

                const auto isGetter = method->getName() == "getStateInformation";

                if (isGetter)
                {
                    diag(method->getLocation(),
                         "'%0()' needs to be implemented to return the plugin's "
                         "current state, otherwise it can't be saved in the host's "
                         "session and the user's work will be lost")
                        << method->getName();
                }
                else
                {
                    diag(method->getLocation(),
                         "'%0()' needs to be implemented to restore the "
                         "plugin's state from the given data, otherwise the "
                         "host's session won't restore properly and the user's "
                         "work will be lost")
                        << method->getName();
                }
            }

        private:
            [[nodiscard]] static bool isPlaceholder(const Stmt* statement)
            {
                if (isa<NullStmt>(statement))
                    return true;

                if (const auto* cast = dyn_cast<CStyleCastExpr>(statement))
                    return cast->getType()->isVoidType();

                if (const auto* call = dyn_cast<CallExpr>(statement))
                    if (const auto* callee = call->getDirectCallee())
                        return utils::qualifiedName(*callee) == "juce::ignoreUnused";

                return false;
            }
        };
    } // namespace empty_state_information

    using empty_state_information::EmptyStateInformationCheck;
} // namespace clang::tidy::juce
