#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringMap.h>

namespace clang::tidy::juce
{
    namespace property_name_case
    {
        class PropertyNameCaseCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto literal = ignoringParenImpCasts(stringLiteral().bind("name"));
                const auto stringArgument = ignoringImplicit(anyOf(literal,
                                                                   cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasAnyName("::juce::String",
                                                                                                                                         "::juce::StringRef")))),
                                                                                    hasArgument(0, literal))));
                const auto parameterClass = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorParameterWithID")));
                const auto attachment = cxxRecordDecl(hasAnyName("::juce::AudioProcessorValueTreeState::SliderAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ButtonAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ComboBoxAttachment"));
                const auto parameterId = [](const auto& matcher) {
                    return expr(matcher).bind("parameter");
                };

                finder->addMatcher(cxxConstructExpr(unless(isInTemplateInstantiation()),
                                                    hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::Identifier")))),
                                                    hasArgument(0, literal)),
                                   this);

                finder->addMatcher(
                    parameterId(cxxConstructExpr(unless(isInTemplateInstantiation()),
                                                 hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::ParameterID")))),
                                                 hasArgument(0, literal))),
                    this);

                finder->addMatcher(
                    parameterId(callExpr(unless(isInTemplateInstantiation()),
                                         callee(functionDecl(hasAnyName("::std::make_unique",
                                                                        "::std::make_shared"),
                                                             hasTemplateArgument(0,
                                                                                 refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(parameterClass))))))),
                                         hasArgument(0, literal))),
                    this);

                finder->addMatcher(
                    parameterId(cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                                  callee(cxxMethodDecl(ofClass(hasName("::juce::AudioProcessorValueTreeState")),
                                                                       hasAnyName("getParameter",
                                                                                  "getRawParameterValue",
                                                                                  "getParameterAsValue",
                                                                                  "getParameterRange",
                                                                                  "addParameterListener",
                                                                                  "removeParameterListener"))),
                                                  hasArgument(0, stringArgument))),
                    this);

                finder->addMatcher(parameterId(cxxConstructExpr(unless(isInTemplateInstantiation()),
                                                                hasType(attachment),
                                                                hasArgument(1, stringArgument))),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* literal = result.Nodes.getNodeAs<StringLiteral>("name");
                const auto isParameter = result.Nodes.getNodeAs<Expr>("parameter") != nullptr;

                if (!literal->isOrdinary()
                    || literal->getLength() < 2
                    || utils::isInMacroBody(literal->getBeginLoc(), *result.SourceManager))
                {
                    return;
                }

                const auto name = literal->getString();
                const auto key = (isParameter ? "parameter:" : "identifier:") + name.lower();
                const auto [first, inserted] = firstSpellings.try_emplace(key,
                                                                          FirstSpelling{
                                                                              .spelling = name.str(),
                                                                              .location = literal->getBeginLoc(),
                                                                          });

                if (inserted || first->second.spelling == name)
                    return;

                diag(literal->getBeginLoc(),
                     "'%0' differs from a previously used "
                     "%select{identifier|parameter ID}2, '%1', only by case. "
                     "This is likely a typo")
                    << name
                    << first->second.spelling
                    << isParameter;

                diag(first->second.location,
                     "'%0' is used here",
                     DiagnosticIDs::Note)
                    << first->second.spelling;
            }

            void onStartOfTranslationUnit() override
            {
                firstSpellings.clear();
            }

        private:
            struct FirstSpelling
            {
                std::string spelling;
                SourceLocation location;
            };

            llvm::StringMap<FirstSpelling> firstSpellings;
        };
    } // namespace property_name_case

    using property_name_case::PropertyNameCaseCheck;
} // namespace clang::tidy::juce
