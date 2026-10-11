#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringExtras.h>

#include <array>
#include <optional>
#include <string>

namespace clang::tidy::juce
{
    namespace hardcoded_resource_provider_root
    {
        class HardcodedResourceProviderRootCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(stringLiteral(unless(isInTemplateInstantiation()))
                                       .bind("literal"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* literal = result.Nodes.getNodeAs<StringLiteral>("literal");
                auto& context = *result.Context;

                if (!literal->isOrdinary()
                    || utils::isInMacroBody(literal->getBeginLoc(),
                                            *result.SourceManager))
                {
                    return;
                }

                for (const auto& root : roots)
                {
                    const auto offset = findRoot(literal->getString(), root.url);

                    if (offset == StringRef::npos)
                        continue;

                    const auto diagnostic = diag(getLocationOfByte(*literal,
                                                                   offset,
                                                                   context),
                                                 "'%0' is only the resource provider root on %1. Use "
                                                 "'juce::WebBrowserComponent::getResourceProviderRoot()' "
                                                 "instead")
                                         << root.url
                                         << root.platforms;

                    if (const auto fix = createFix(*literal,
                                                   offset,
                                                   root.url,
                                                   context))
                    {
                        diagnostic << *fix;
                    }
                }
            }

        private:
            struct Root
            {
                StringRef url;
                StringRef platforms;
            };

            struct Delimiters
            {
                std::string open;
                std::string close;
            };

            static inline constexpr std::array roots{
                Root{
                    .url = "juce://juce.backend",
                    .platforms = "macOS, iOS and Linux",
                },
                Root{
                    .url = "https://juce.backend",
                    .platforms = "Windows and Android",
                },
            };

            static inline constexpr std::array stringOperators{
                OO_EqualEqual,
                OO_ExclaimEqual,
                OO_Plus,
                OO_PlusEqual,
                OO_LessLess,
            };

            static inline constexpr auto rootExpression = "juce::WebBrowserComponent::getResourceProviderRoot()";

            [[nodiscard]] static std::optional<FixItHint> createFix(const StringLiteral& literal,
                                                                    size_t offset,
                                                                    StringRef root,
                                                                    ASTContext& context)
            {
                const auto text = literal.getString();
                const auto hasSlash = text.substr(offset + root.size()).starts_with("/");
                const auto end = offset + root.size() + (hasSlash ? 1 : 0);
                const auto before = text.take_front(offset);
                const auto after = text.drop_front(end);

                if (literal.getBeginLoc().isMacroID()
                    || literal.getNumConcatenated() != 1
                    || !acceptsString(literal, before.empty() && after.empty() && hasSlash, context))
                {
                    return std::nullopt;
                }

                const auto& sourceManager = context.getSourceManager();
                const auto delimiters = getDelimiters(Lexer::getSourceText(CharSourceRange::getTokenRange(literal.getSourceRange()),
                                                                           sourceManager,
                                                                           context.getLangOpts()));

                if (!delimiters)
                    return std::nullopt;

                auto replacement = std::string{ rootExpression } + (hasSlash ? "" : ".dropLastCharacters(1)");

                if (!before.empty())
                    replacement = delimiters->close + " + " + replacement;

                if (!after.empty())
                    replacement += " + " + delimiters->open;

                const auto start = before.empty()
                                     ? literal.getBeginLoc()
                                     : getLocationOfByte(literal, offset, context);
                const auto finish = after.empty()
                                      ? Lexer::getLocForEndOfToken(literal.getEndLoc(), 0, sourceManager, context.getLangOpts())
                                      : getLocationOfByte(literal, end, context);

                return FixItHint::CreateReplacement(CharSourceRange::getCharRange(start, finish), replacement);
            }

            [[nodiscard]] static bool acceptsString(const StringLiteral& literal, bool isWholeRoot, ASTContext& context)
            {
                const auto* parent = getParentSkippingImplicit(literal, context);

                if (const auto* construction = dyn_cast_or_null<CXXConstructExpr>(parent))
                {
                    const auto name = utils::qualifiedName(*construction->getConstructor()->getParent());

                    if (construction->getNumArgs() != 1)
                        return false;

                    if (name == "juce::String" || name == "juce::var")
                        return true;

                    return name == "juce::StringRef"
                        && (isWholeRoot || isa_and_nonnull<CallExpr, CXXConstructExpr>(getParentSkippingImplicit(*construction, context)));
                }

                const auto* call = dyn_cast_or_null<CallExpr>(parent);
                const auto* function = call != nullptr ? call->getDirectCallee() : nullptr;

                return function != nullptr
                    && llvm::is_contained(stringOperators, function->getOverloadedOperator())
                    && llvm::any_of(call->arguments(), [&literal](const Expr* argument) {
                           return argument->IgnoreParenImpCasts() != &literal && isJuceString(argument->getType());
                       });
            }

            [[nodiscard]] static const Expr* getParentSkippingImplicit(const Expr& expression, ASTContext& context)
            {
                const auto* child = &expression;

                while (true)
                {
                    const auto parents = context.getParents(*child);
                    const auto* parent = !parents.empty() ? parents.begin()->get<Expr>() : nullptr;

                    if (!isa_and_nonnull<ImplicitCastExpr, ParenExpr, MaterializeTemporaryExpr, CXXBindTemporaryExpr>(parent))
                        return parent;

                    child = parent;
                }
            }

            [[nodiscard]] static bool isJuceString(QualType type)
            {
                const auto* record = type.getNonReferenceType()->getAsCXXRecordDecl();
                return record != nullptr && utils::qualifiedName(*record) == "juce::String";
            }

            [[nodiscard]] static std::optional<Delimiters> getDelimiters(StringRef spelling)
            {
                if (spelling.starts_with("\""))
                    return Delimiters{ .open = "\"", .close = "\"" };

                if (!spelling.consume_front("R\""))
                    return std::nullopt;

                const auto delimiter = spelling.take_until([](char character) {
                    return character == '(';
                });

                return Delimiters{ .open = "R\"" + delimiter.str() + "(", .close = ")" + delimiter.str() + "\"" };
            }

            [[nodiscard]] static SourceLocation getLocationOfByte(const StringLiteral& literal, size_t offset, const ASTContext& context)
            {
                return literal.getLocationOfByte(static_cast<unsigned int>(offset),
                                                 context.getSourceManager(),
                                                 context.getLangOpts(),
                                                 context.getTargetInfo());
            }

            [[nodiscard]] static size_t findRoot(StringRef text, StringRef root)
            {
                for (auto offset = text.find(root); offset != StringRef::npos; offset = text.find(root, offset + 1))
                {
                    const auto rest = text.drop_front(offset + root.size());

                    if (rest.empty()
                        || !(llvm::isAlnum(rest.front()) || rest.front() == '.' || rest.front() == '-'))
                    {
                        return offset;
                    }
                }

                return StringRef::npos;
            }
        };
    } // namespace hardcoded_resource_provider_root

    using hardcoded_resource_provider_root::HardcodedResourceProviderRootCheck;
} // namespace clang::tidy::juce
