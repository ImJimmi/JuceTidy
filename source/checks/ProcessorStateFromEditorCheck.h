#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace processor_state_from_editor
    {
        class ProcessorStateFromEditorCheck : public JuceTidyCheck
        {
        public:
            ProcessorStateFromEditorCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawProcessorMethods{ Options.get("ProcessorMethods", defaultProcessorMethods) }
                , processorMethods{ utils::parseQualifiedNames(rawProcessorMethods) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "ProcessorMethods", rawProcessorMethods);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto editor = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorEditor")));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(utils::isOrOverridesAnyOf(processorMethods)).bind("method")),
                                      hasAncestor(cxxMethodDecl(ofClass(editor))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                diag(call->getExprLoc(),
                     "don't call '%0()' from the editor or it will change the "
                     "behavior of the plugin depending on whether the editor "
                     "is open or not")
                    << method->getName();
            }

        private:
            static inline constexpr auto defaultProcessorMethods =
                "::juce::AudioProcessor::setLatencySamples;"
                "::juce::AudioProcessor::setPlayConfigDetails;"
                "::juce::AudioProcessor::setRateAndBufferSizeDetails";

            const StringRef rawProcessorMethods;
            const std::vector<std::string> processorMethods;
        };
    } // namespace processor_state_from_editor

    using processor_state_from_editor::ProcessorStateFromEditorCheck;
} // namespace clang::tidy::juce
