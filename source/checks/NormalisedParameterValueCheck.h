#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace normalised_parameter_value
    {
        class NormalisedParameterValueCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto rangedParameter = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::RangedAudioParameter")));
                const auto normalisedConsumer = callExpr(callee(functionDecl(hasAnyName("convertFrom0to1",
                                                                                        "setValueNotifyingHost",
                                                                                        "setValue",
                                                                                        "getText"))));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("getValue"),
                                                           ofClass(hasName("::juce::AudioProcessorParameter")))),
                                      on(expr(anyOf(hasType(rangedParameter),
                                                    hasType(pointsTo(rangedParameter))))),
                                      unless(hasAncestor(normalisedConsumer)),
                                      unless(forCallable(cxxMethodDecl(ofClass(rangedParameter)))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                diag(call->getExprLoc(),
                     "'getValue()' returns the parameter's raw value from 0-1. "
                     "Use 'convertFrom0to1()', 'get()', or 'operator*()', or "
                     "use a generic 'juce::AudioProcessorParameter' if you "
                     "really do want the raw value");
            }
        };
    } // namespace normalised_parameter_value

    using normalised_parameter_value::NormalisedParameterValueCheck;
} // namespace clang::tidy::juce
