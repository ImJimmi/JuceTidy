#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace repaint_timer
    {
        class RepaintTimerCheck : public JuceTidyCheck
        {
        public:
            RepaintTimerCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , minimumFrequency{ Options.get("MinimumFrequency", 30u) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "MinimumFrequency", minimumFrequency);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto repaint = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("repaint"),
                                                                            ofClass(hasName("::juce::Component")))),
                                                       on(ignoringParenImpCasts(cxxThisExpr())));

                finder->addMatcher(
                    cxxMethodDecl(isDefinition(),
                                  unless(isInstantiated()),
                                  utils::isOrOverridesAnyOf({ "juce::Timer::timerCallback" }),
                                  ofClass(cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::Component"))).bind("owner")),
                                  hasBody(compoundStmt(statementCountIs(1),
                                                       has(repaint))))
                        .bind("callback"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* callback = result.Nodes.getNodeAs<CXXMethodDecl>("callback");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");

                if (callback->getLocation().isMacroID()
                    || !startsFastTimer(*owner, *result.Context))
                {
                    return;
                }

                diag(callback->getLocation(),
                     "don't use a timer to repaint a component as different "
                     "displays have different refresh rates. Instead, use "
                     "juce::VBlankAttachment to get callbacks at the current "
                     "display's actual refresh rate")
                    << callback;
            }

        private:
            [[nodiscard]] bool startsFastTimer(const CXXRecordDecl& record, ASTContext& context) const
            {
                using namespace clang::ast_matchers;

                const auto start = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("startTimer", "startTimerHz"),
                                                                          ofClass(hasName("::juce::Timer")))
                                                                .bind("method")),
                                                     on(ignoringParenImpCasts(cxxThisExpr())))
                                       .bind("start");

                for (const auto* method : record.methods())
                {
                    const FunctionDecl* definition = nullptr;

                    if (!method->hasBody(definition) || definition == nullptr)
                        continue;

                    for (const auto* statement : utils::getFunctionStatements(*definition))
                    {
                        for (const auto& nodes : match(findAll(start), *statement, context))
                        {
                            const auto* call = nodes.getNodeAs<CXXMemberCallExpr>("start");
                            const auto* startMethod = nodes.getNodeAs<CXXMethodDecl>("method");

                            if (call->getNumArgs() == 0)
                                continue;

                            const auto value = call->getArg(0)->getIntegerConstantExpr(context);

                            if (!value.has_value() || value->getExtValue() <= 0)
                                continue;

                            const auto frequency = startMethod->getName() == "startTimerHz"
                                                     ? static_cast<double>(value->getExtValue())
                                                     : 1000.0 / static_cast<double>(value->getExtValue());

                            if (frequency >= minimumFrequency)
                                return true;
                        }
                    }
                }

                return false;
            }

            const unsigned int minimumFrequency;
        };
    } // namespace repaint_timer

    using repaint_timer::RepaintTimerCheck;
} // namespace clang::tidy::juce
