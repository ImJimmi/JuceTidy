#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringSet.h>

namespace clang::tidy::juce
{
    namespace unknown_parameter_id
    {
        class UnknownParameterIdCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto literal = ignoringParenImpCasts(stringLiteral().bind("id"));
                const auto stringArgument = ignoringImplicit(anyOf(literal,
                                                                   cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasAnyName("::juce::String",
                                                                                                                                         "::juce::StringRef")))),
                                                                                    hasArgument(0, literal))));
                const auto parameterClass = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorParameterWithID")));
                const auto parameterIdClass = cxxRecordDecl(hasName("::juce::ParameterID"));
                const auto attachment = cxxRecordDecl(hasAnyName("::juce::AudioProcessorValueTreeState::SliderAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ButtonAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ComboBoxAttachment"));
                const auto declared = [](const auto& matcher) {
                    return expr(matcher).bind("declaration");
                };

                finder->addMatcher(
                    declared(cxxConstructExpr(unless(isInTemplateInstantiation()),
                                              hasDeclaration(cxxConstructorDecl(ofClass(parameterIdClass),
                                                                                unless(isCopyConstructor()),
                                                                                unless(isMoveConstructor()),
                                                                                unless(isDefaultConstructor()))),
                                              optionally(hasArgument(0, literal)))),
                    this);

                finder->addMatcher(
                    declared(callExpr(unless(isInTemplateInstantiation()),
                                      callee(functionDecl(hasAnyName("::std::make_unique",
                                                                     "::std::make_shared"),
                                                          hasTemplateArgument(0,
                                                                              refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(parameterClass))))))),
                                      hasArgument(0,
                                                  expr(unless(hasType(parameterIdClass)),
                                                       optionally(literal))))),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(ofClass(hasName("::juce::AudioProcessorValueTreeState")),
                                                           hasAnyName("getParameter",
                                                                      "getRawParameterValue",
                                                                      "getParameterAsValue",
                                                                      "getParameterRange",
                                                                      "addParameterListener",
                                                                      "removeParameterListener"))),
                                      hasArgument(0, stringArgument))
                        .bind("use"),
                    this);

                finder->addMatcher(cxxConstructExpr(unless(isInTemplateInstantiation()),
                                                    hasType(attachment),
                                                    hasArgument(1, stringArgument))
                                       .bind("use"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* literal = result.Nodes.getNodeAs<StringLiteral>("id");

                if (result.Nodes.getNodeAs<Expr>("declaration") != nullptr)
                {
                    if (literal != nullptr && literal->isOrdinary())
                        declaredIds.insert(literal->getString());
                    else
                        hasDynamicIds = true;

                    return;
                }

                if (literal != nullptr
                    && literal->isOrdinary()
                    && !utils::isInMacroBody(literal->getBeginLoc(), *result.SourceManager))
                {
                    uses.push_back({
                        .id = literal->getString().str(),
                        .location = literal->getBeginLoc(),
                    });
                }
            }

            void onStartOfTranslationUnit() override
            {
                declaredIds.clear();
                uses.clear();
                hasDynamicIds = false;
            }

            void onEndOfTranslationUnit() override
            {
                if (declaredIds.empty() || hasDynamicIds)
                    return;

                for (const auto& [id, location] : uses)
                {
                    if (declaredIds.contains(id))
                        continue;

                    const auto closest = llvm::min_element(declaredIds.keys(),
                                                           [&id](StringRef a, StringRef b) {
                                                               return StringRef(id).edit_distance(a) < StringRef(id).edit_distance(b);
                                                           });
                    const auto suggestion = StringRef(id).edit_distance(*closest) <= 2
                                              ? (". Did you mean \"" + *closest + "\"?").str()
                                              : std::string{};

                    diag(location,
                         "no parameter with the ID \"%0\"%1")
                        << id
                        << suggestion;
                }
            }

        private:
            struct Use
            {
                std::string id;
                SourceLocation location;
            };

            llvm::StringSet<> declaredIds;
            std::vector<Use> uses;
            bool hasDynamicIds = false;
        };
    } // namespace unknown_parameter_id

    using unknown_parameter_id::UnknownParameterIdCheck;
} // namespace clang::tidy::juce
