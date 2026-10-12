#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unchecked_sample_rate
    {
        class UncheckedSampleRateCheck : public JuceTidyCheck
        {
        public:
            UncheckedSampleRateCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawSampleRateFunctions{ Options.get("SampleRateFunctions", defaultSampleRateFunctions) }
                , sampleRateFunctions{ utils::parseQualifiedNames(rawSampleRateFunctions) }
                , rawPreparedFunctions{ Options.get("PreparedFunctions", defaultPreparedFunctions) }
                , preparedFunctions{ utils::parseQualifiedNames(rawPreparedFunctions) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "SampleRateFunctions", rawSampleRateFunctions);
                Options.store(options, "PreparedFunctions", rawPreparedFunctions);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto sampleRate = callExpr(callee(functionDecl(utils::isOrOverridesAnyOf(sampleRateFunctions))));
                const auto storedSampleRate = declRefExpr(to(varDecl(hasLocalStorage(),
                                                                     hasInitializer(ignoringParenCasts(sampleRate)))
                                                                 .bind("variable")));

                finder->addMatcher(
                    binaryOperator(unless(isInTemplateInstantiation()),
                                   hasAnyOperatorName("/", "/=", "%", "%="),
                                   hasRHS(ignoringParenCasts(expr(anyOf(sampleRate, storedSampleRate)).bind("divisor"))),
                                   forCallable(functionDecl(unless(utils::isOrOverridesAnyOf(preparedFunctions)),
                                                            hasBody(stmt().bind("body")))))
                        .bind("division"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* divisor = result.Nodes.getNodeAs<Expr>("divisor");
                const auto* body = result.Nodes.getNodeAs<Stmt>("body");

                if (utils::isInMacroBody(divisor->getExprLoc(), *result.SourceManager))
                    return;

                if (const auto* variable = result.Nodes.getNodeAs<VarDecl>("variable"))
                {
                    const auto reference = ignoringParenCasts(declRefExpr(to(varDecl(equalsNode(variable)))));
                    const auto comparison = expr(anyOf(binaryOperator(isComparisonOperator(),
                                                                      hasEitherOperand(reference)),
                                                       callExpr(hasAnyArgument(reference),
                                                                callee(functionDecl(hasAnyName("approximatelyEqual",
                                                                                               "exactlyEqual",
                                                                                               "isPositiveAndBelow",
                                                                                               "isPositiveAndNotGreaterThan"))))));

                    if (utils::findFirst(comparison, *body, *result.Context) != nullptr)
                        return;
                }

                diag(divisor->getExprLoc(),
                     "the sample rate could be 0 here if the processor hasn't "
                     "been initialised yet by the host, so you'd be dividing "
                     "by zero. Check the sample rate is greater than 0 and "
                     "explicitly handle the case where it's not");
            }

        private:
            static inline constexpr auto defaultSampleRateFunctions =
                "::juce::AudioProcessor::getSampleRate;"
                "::juce::AudioIODevice::getCurrentSampleRate";
            static inline constexpr auto defaultPreparedFunctions =
                "::juce::AudioProcessor::prepareToPlay;"
                "::juce::AudioProcessor::processBlock;"
                "::juce::AudioProcessor::processBlockBypassed";

            const StringRef rawSampleRateFunctions;
            const std::vector<std::string> sampleRateFunctions;
            const StringRef rawPreparedFunctions;
            const std::vector<std::string> preparedFunctions;
        };
    } // namespace unchecked_sample_rate

    using unchecked_sample_rate::UncheckedSampleRateCheck;
} // namespace clang::tidy::juce
