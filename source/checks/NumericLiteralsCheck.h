#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringExtras.h>

namespace clang::tidy::juce
{
    namespace numeric_literals
    {
        class NumericLiteralsCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(expr(anyOf(floatLiteral(),
                                              integerLiteral()),
                                        unless(hasParent(userDefinedLiteral())))
                                       .bind("literal"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* literal = result.Nodes.getNodeAs<Expr>("literal");
                const auto& sourceManager = *result.SourceManager;

                if (utils::isInMacroBody(literal->getBeginLoc(), sourceManager))
                    return;

                const auto range = CharSourceRange::getTokenRange(sourceManager.getSpellingLoc(literal->getBeginLoc()));
                const auto spelling = Lexer::getSourceText(range, sourceManager, result.Context->getLangOpts());

                if (isa<FloatingLiteral>(literal))
                {
                    if (const auto fixed = fixFloatingPoint(spelling))
                    {
                        diag(literal->getBeginLoc(),
                             "floating-point literal should have digits both "
                             "before and after the decimal point")
                            << FixItHint::CreateReplacement(range, *fixed);
                    }

                    return;
                }

                if (const auto fixed = fixHexadecimal(spelling))
                {
                    diag(literal->getBeginLoc(),
                         "hexadecimal literal should be written in lowercase")
                        << FixItHint::CreateReplacement(range, *fixed);
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            [[nodiscard]] static bool isHexadecimal(StringRef spelling)
            {
                return spelling.starts_with_insensitive("0x");
            }

            [[nodiscard]] static std::optional<std::string> fixFloatingPoint(StringRef spelling)
            {
                const auto point = spelling.find('.');

                if (isHexadecimal(spelling) || point == StringRef::npos)
                    return std::nullopt;

                auto fixed = spelling.str();

                if (const auto fraction = spelling.drop_front(point + 1);
                    fraction.empty() || !llvm::isDigit(fraction.front()))
                {
                    fixed.insert(point + 1, "0");
                }

                if (point == 0)
                    fixed.insert(0, "0");

                if (fixed == spelling)
                    return std::nullopt;

                return fixed;
            }

            [[nodiscard]] static std::optional<std::string> fixHexadecimal(StringRef spelling)
            {
                if (!isHexadecimal(spelling))
                    return std::nullopt;

                const auto suffixStart = spelling.find_if_not(
                    [](char c) {
                        return llvm::isHexDigit(c) || c == '\'';
                    },
                    2);
                const auto digits = spelling.take_front(suffixStart);
                auto fixed = digits.lower() + spelling.drop_front(digits.size()).str();

                if (fixed == spelling)
                    return std::nullopt;

                return fixed;
            }
        };
    } // namespace numeric_literals

    using numeric_literals::NumericLiteralsCheck;
} // namespace clang::tidy::juce
