#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace nullable_dereference
    {
        class NullableDereferenceCheck : public JuceTidyCheck
        {
        public:
            NullableDereferenceCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawNullableFunctions{ Options.get("NullableFunctions", defaultNullableFunctions) }
                , nullableFunctions{ utils::parseQualifiedNames(rawNullableFunctions) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "NullableFunctions", rawNullableFunctions);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto nullableCall = callExpr(callee(functionDecl(utils::isOrOverridesAnyOf(nullableFunctions))
                                                              .bind("function")))
                                              .bind("call");

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(memberExpr(isArrow(),
                                          hasObjectExpression(ignoringParenImpCasts(nullableCall))),
                               unaryOperator(hasOperatorName("*"),
                                             hasUnaryOperand(ignoringParenImpCasts(nullableCall))),
                               cxxOperatorCallExpr(hasAnyOverloadedOperatorName("->", "*"),
                                                   hasArgument(0, ignoringImplicit(nullableCall))))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CallExpr>("call");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");

                diag(call->getExprLoc(),
                     "%0 may return nullptr. Check the result before "
                     "dereferencing it")
                    << function;
            }

        private:
            static inline constexpr auto defaultNullableFunctions =
                "::juce::AudioProcessor::getPlayHead;"
                "::juce::AudioProcessor::getActiveEditor;"
                "::juce::AudioProcessorValueTreeState::getParameter;"
                "::juce::AudioProcessorValueTreeState::getRawParameterValue;"
                "::juce::Component::getParentComponent;"
                "::juce::Component::findParentComponentOfClass;"
                "::juce::Component::getPeer;"
                "::juce::MessageManager::getInstanceWithoutCreating;"
                "::juce::AudioDeviceManager::getCurrentAudioDevice;"
                "::juce::Displays::getPrimaryDisplay;"
                "::juce::AudioProcessor::getXmlFromBinary;"
                "::juce::parseXML;"
                "::juce::parseXMLIfTagMatches;"
                "::juce::XmlDocument::parse;"
                "::juce::XmlDocument::getDocumentElement;"
                "::juce::ValueTree::createXml;"
                "::juce::WebBrowserComponent::EvaluationResult::getResult;"
                "::juce::WebBrowserComponent::EvaluationResult::getError";

            const StringRef rawNullableFunctions;
            const std::vector<std::string> nullableFunctions;
        };
    } // namespace nullable_dereference

    using nullable_dereference::NullableDereferenceCheck;
} // namespace clang::tidy::juce
