#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace prefer_timed_callback
    {
        class PreferTimedCallbackCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxRecordDecl(isDefinition(),
                                  unless(isExpansionInSystemHeader()),
                                  unless(isTemplateInstantiation()),
                                  isDirectlyDerivedFrom(hasName("::juce::Timer")))
                        .bind("record"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");

                if (utils::isInJuceNamespace(*record))
                    return;

                for (const auto& base : record->bases())
                {
                    const auto* baseRecord = base.getType()->getAsCXXRecordDecl();

                    if (baseRecord == nullptr || utils::qualifiedName(*baseRecord) != "juce::Timer" || base.getBeginLoc().isMacroID())
                        continue;

                    diag(base.getBeginLoc(),
                         "use a 'juce::TimedCallback' member instead of "
                         "inheriting from 'juce::Timer'");
                }
            }
        };
    } // namespace prefer_timed_callback

    using prefer_timed_callback::PreferTimedCallbackCheck;
} // namespace clang::tidy::juce
