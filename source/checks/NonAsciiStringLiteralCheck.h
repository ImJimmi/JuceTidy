#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace non_ascii_string_literal
    {
        class NonAsciiStringLiteralCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::String")),
                                                                       hasParameter(0, hasType(asString("const char *"))))),
                                     hasArgument(0, ignoringParenImpCasts(stringLiteral().bind("literal"))))
                        .bind("construction"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* construction = result.Nodes.getNodeAs<CXXConstructExpr>("construction");
                const auto* literal = result.Nodes.getNodeAs<StringLiteral>("literal");
                const auto& sourceManager = *result.SourceManager;

                if (utils::isInMacroBody(literal->getBeginLoc(), sourceManager))
                    return;

                if (llvm::all_of(literal->getBytes(),
                                 [](char character) {
                                     return static_cast<unsigned char>(character) < 0x80;
                                 }))
                {
                    return;
                }

                const auto diagnostic = diag(literal->getBeginLoc(),
                                             "constructing a 'juce::String' "
                                             "from a string literal containing "
                                             "non-ASCII characters. Use "
                                             "'juce::String::fromUTF8()' "
                                             "instead");

                const auto range = utils::getFileRange(literal->getSourceRange(), sourceManager, result.Context->getLangOpts());

                if (range && construction->getNumArgs() == 1)
                {
                    diagnostic << FixItHint::CreateReplacement(*range,
                                                               "juce::String::fromUTF8("
                                                                   + Lexer::getSourceText(*range, sourceManager, result.Context->getLangOpts()).str()
                                                                   + ")");
                }
            }
        };
    } // namespace non_ascii_string_literal

    using non_ascii_string_literal::NonAsciiStringLiteralCheck;
} // namespace clang::tidy::juce
