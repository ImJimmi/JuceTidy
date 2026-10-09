#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace stop_in_destructor
    {
        class StopInDestructorCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                auto& context = *result.Context;

                for (const auto& [callback, stopFunction] : rules)
                {
                    const auto method = llvm::find_if(record->methods(),
                                                      [&callback](const CXXMethodDecl* candidate) {
                                                          return candidate->size_overridden_methods() > 0
                                                              && utils::isOrOverridesAnyOf(*candidate, { callback.str() });
                                                      });

                    if (method == record->method_end())
                        continue;

                    const auto callsStop = callExpr(callee(functionDecl(hasName(stopFunction))));
                    const auto stops = callExpr(anyOf(callsStop,
                                                      callExpr(callee(functionDecl(hasBody(hasDescendant(callsStop)))))));

                    if (utils::searchDestructor(*record, stops, context) != utils::DestructorSearch::notFound)
                        continue;

                    diag(record->getLocation(),
                         "%0 overrides %1 but its destructor doesn't call "
                         "'%2'. The callback could be called while the object "
                         "is being destroyed")
                        << record
                        << *method
                        << stopFunction;
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            struct Rule
            {
                StringRef callback;
                StringRef stopFunction;
            };

            static inline constexpr std::array rules{
                Rule{
                    .callback = "juce::Thread::run",
                    .stopFunction = "stopThread",
                },
                Rule{
                    .callback = "juce::HighResolutionTimer::hiResTimerCallback",
                    .stopFunction = "stopTimer",
                },
            };
        };
    } // namespace stop_in_destructor

    using stop_in_destructor::StopInDestructorCheck;
} // namespace clang::tidy::juce
