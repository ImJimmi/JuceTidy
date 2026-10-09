#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace identifier_from_literal
    {
        class IdentifierFromLiteralCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::Identifier")))),
                                     hasArgument(0, ignoringParenImpCasts(stringLiteral())),
                                     unless(hasAncestor(varDecl(hasStaticStorageDuration()))),
                                     unless(hasAncestor(fieldDecl())))
                        .bind("construction"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* construction = result.Nodes.getNodeAs<CXXConstructExpr>("construction");

                if (utils::isInMacroBody(construction->getBeginLoc(), *result.SourceManager))
                    return;

                diag(construction->getBeginLoc(),
                     "constructing a juce::Identifier from a string literal is "
                     "expensive. Declare a static juce::Identifier and reuse "
                     "it");
            }
        };
    } // namespace identifier_from_literal

    using identifier_from_literal::IdentifierFromLiteralCheck;
} // namespace clang::tidy::juce
