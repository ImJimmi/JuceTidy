#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/Lex/Preprocessor.h>

#include <array>

namespace clang::tidy::juce
{
    namespace platform_macros
    {
        class PlatformMacrosCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                preprocessor->addPPCallbacks(std::make_unique<utils::ConditionalDirectiveListener>(
                    sourceManager,
                    getLangOpts(),
                    [this, preprocessor, &sourceManager](const utils::ConditionalDirective& directive) {
                        checkDirective(directive, *preprocessor, sourceManager);
                    }));
            }

        private:
            struct JuceMacro
            {
                StringRef macro;
                StringRef replacement;
            };

            static inline constexpr std::array juceMacros{
                JuceMacro{ .macro = "_WIN32", .replacement = "JUCE_WINDOWS" },
                JuceMacro{ .macro = "WIN32", .replacement = "JUCE_WINDOWS" },
                JuceMacro{ .macro = "_WINDOWS", .replacement = "JUCE_WINDOWS" },
                JuceMacro{ .macro = "__WIN32__", .replacement = "JUCE_WINDOWS" },
                JuceMacro{ .macro = "_WIN64", .replacement = "JUCE_WINDOWS && JUCE_64BIT" },
                JuceMacro{ .macro = "__MINGW32__", .replacement = "JUCE_MINGW" },
                JuceMacro{ .macro = "__MINGW64__", .replacement = "JUCE_MINGW" },
                JuceMacro{ .macro = "__APPLE__", .replacement = "JUCE_MAC || JUCE_IOS" },
                JuceMacro{ .macro = "__APPLE_CC__", .replacement = "JUCE_MAC || JUCE_IOS" },
                JuceMacro{ .macro = "__APPLE_CPP__", .replacement = "JUCE_MAC || JUCE_IOS" },
                JuceMacro{ .macro = "TARGET_OS_MAC", .replacement = "JUCE_MAC || JUCE_IOS" },
                JuceMacro{ .macro = "TARGET_OS_OSX", .replacement = "JUCE_MAC" },
                JuceMacro{ .macro = "TARGET_OS_IPHONE", .replacement = "JUCE_IOS" },
                JuceMacro{ .macro = "TARGET_OS_IOS", .replacement = "JUCE_IOS" },
                JuceMacro{ .macro = "TARGET_IPHONE_SIMULATOR", .replacement = "JUCE_IOS" },
                JuceMacro{ .macro = "TARGET_OS_SIMULATOR", .replacement = "JUCE_IOS" },
                JuceMacro{ .macro = "__linux__", .replacement = "JUCE_LINUX" },
                JuceMacro{ .macro = "__linux", .replacement = "JUCE_LINUX" },
                JuceMacro{ .macro = "LINUX", .replacement = "JUCE_LINUX" },
                JuceMacro{ .macro = "__ANDROID__", .replacement = "JUCE_ANDROID" },
                JuceMacro{ .macro = "__FreeBSD__", .replacement = "JUCE_BSD" },
                JuceMacro{ .macro = "__OpenBSD__", .replacement = "JUCE_BSD" },
                JuceMacro{ .macro = "__wasm__", .replacement = "JUCE_WASM" },
                JuceMacro{ .macro = "__EMSCRIPTEN__", .replacement = "JUCE_WASM" },
                JuceMacro{ .macro = "_DEBUG", .replacement = "JUCE_DEBUG" },
                JuceMacro{ .macro = "DEBUG", .replacement = "JUCE_DEBUG" },
                JuceMacro{ .macro = "NDEBUG", .replacement = "!JUCE_DEBUG" },
                JuceMacro{ .macro = "__clang__", .replacement = "JUCE_CLANG" },
                JuceMacro{ .macro = "__GNUC__", .replacement = "JUCE_GCC || JUCE_CLANG" },
                JuceMacro{ .macro = "_MSC_VER", .replacement = "JUCE_MSVC" },
                JuceMacro{ .macro = "__x86_64__", .replacement = "JUCE_INTEL && JUCE_64BIT" },
                JuceMacro{ .macro = "__amd64__", .replacement = "JUCE_INTEL && JUCE_64BIT" },
                JuceMacro{ .macro = "_M_X64", .replacement = "JUCE_INTEL && JUCE_64BIT" },
                JuceMacro{ .macro = "_M_AMD64", .replacement = "JUCE_INTEL && JUCE_64BIT" },
                JuceMacro{ .macro = "__i386__", .replacement = "JUCE_INTEL && JUCE_32BIT" },
                JuceMacro{ .macro = "_M_IX86", .replacement = "JUCE_INTEL && JUCE_32BIT" },
                JuceMacro{ .macro = "__aarch64__", .replacement = "JUCE_ARM && JUCE_64BIT" },
                JuceMacro{ .macro = "__arm64__", .replacement = "JUCE_ARM && JUCE_64BIT" },
                JuceMacro{ .macro = "_M_ARM64", .replacement = "JUCE_ARM && JUCE_64BIT" },
                JuceMacro{ .macro = "__arm__", .replacement = "JUCE_ARM && JUCE_32BIT" },
                JuceMacro{ .macro = "_M_ARM", .replacement = "JUCE_ARM && JUCE_32BIT" },
                JuceMacro{ .macro = "__LP64__", .replacement = "JUCE_64BIT" },
                JuceMacro{ .macro = "_LP64", .replacement = "JUCE_64BIT" },
                JuceMacro{ .macro = "__LITTLE_ENDIAN__", .replacement = "JUCE_LITTLE_ENDIAN" },
                JuceMacro{ .macro = "__BIG_ENDIAN__", .replacement = "JUCE_BIG_ENDIAN" },
            };

            static inline constexpr std::array juceTargetPlatforms{
                "JUCE_WINDOWS",
                "JUCE_MAC",
                "JUCE_IOS",
                "JUCE_LINUX",
                "JUCE_ANDROID",
                "JUCE_BSD",
                "JUCE_WASM",
            };

            void checkDirective(const utils::ConditionalDirective& directive,
                                Preprocessor& preprocessor,
                                const SourceManager& sourceManager)
            {
                const auto juceIsIncluded = llvm::any_of(juceTargetPlatforms,
                                                         [&preprocessor](StringRef platform) {
                                                             return preprocessor.isMacroDefined(platform);
                                                         });

                if (!juceIsIncluded)
                    return;

                const auto report = [this](const Token& name,
                                           const JuceMacro& macro,
                                           CharSourceRange range,
                                           StringRef replacement) {
                    diag(name.getLocation(),
                         "use '%0' rather than '%1'")
                        << macro.replacement
                        << macro.macro
                        << FixItHint::CreateReplacement(range, replacement);
                };

                const auto& tokens = directive.tokens;

                if (directive.isDefinedCheck())
                {
                    const auto name = utils::getRawSpelling(tokens.at(1));
                    const auto* macro = findJuceMacro(name);

                    if (macro == nullptr || utils::isFollowedByDefinitionOf(directive, name, sourceManager))
                        return;

                    const auto* const keyword = directive.keyword().starts_with("el") ? "#elif " : "#if ";
                    const auto condition = directive.keyword().ends_with("ndef")
                                             ? negate(macro->replacement)
                                             : macro->replacement.str();
                    const auto range = CharSourceRange::getCharRange(getHashLocation(tokens.at(0).getLocation(), sourceManager),
                                                                     tokens.at(1).getEndLoc());
                    report(tokens.at(1), *macro, range, keyword + condition);
                    return;
                }

                for (std::size_t index = 1; index < tokens.size(); ++index)
                {
                    const auto defined = utils::matchDefinedOperator(tokens, index);
                    const auto nameIndex = defined ? defined->name : index;
                    const auto last = defined ? defined->last : index;
                    const auto name = utils::getRawSpelling(tokens.at(nameIndex));
                    const auto* macro = tokens.at(nameIndex).is(tok::raw_identifier)
                                          ? findJuceMacro(name)
                                          : nullptr;
                    const auto isCompared = !defined
                                         && (isComparison(tokens.at(index - 1))
                                             || (index + 1 < tokens.size() && isComparison(tokens.at(index + 1))));

                    if (macro != nullptr
                        && !isCompared
                        && !(defined && utils::isFollowedByDefinitionOf(directive, name, sourceManager)))
                    {
                        const auto isNegated = tokens.at(index - 1).is(tok::exclaim);
                        const auto first = isNegated ? index - 1 : index;
                        const auto isWholeCondition = first == 1 && last == tokens.size() - 1;
                        const auto condition = isNegated
                                                 ? negate(macro->replacement)
                                                 : macro->replacement.str();
                        const auto replacement = isCompound(condition) && !isWholeCondition
                                                   ? "(" + condition + ")"
                                                   : condition;

                        report(tokens.at(nameIndex),
                               *macro,
                               utils::getTokensRange(tokens.at(first), tokens.at(last)),
                               replacement);
                    }

                    index = last;
                }
            }

            [[nodiscard]] static const JuceMacro* findJuceMacro(StringRef macro)
            {
                const auto* found = llvm::find_if(juceMacros,
                                                  [macro](const JuceMacro& entry) {
                                                      return entry.macro == macro;
                                                  });
                return found != juceMacros.end() ? found : nullptr;
            }

            [[nodiscard]] static bool isCompound(StringRef expression)
            {
                return expression.contains("||") || expression.contains("&&");
            }

            [[nodiscard]] static std::string negate(StringRef expression)
            {
                if (expression.consume_front("!"))
                    return expression.str();

                return isCompound(expression)
                         ? ("!(" + expression + ")").str()
                         : ("!" + expression).str();
            }

            [[nodiscard]] static bool isComparison(const Token& token)
            {
                return token.isOneOf(tok::equalequal,
                                     tok::exclaimequal,
                                     tok::less,
                                     tok::greater,
                                     tok::lessequal,
                                     tok::greaterequal);
            }

            [[nodiscard]] static SourceLocation getHashLocation(SourceLocation keyword, const SourceManager& sourceManager)
            {
                const auto column = static_cast<int>(sourceManager.getSpellingColumnNumber(keyword));
                const auto indentation = static_cast<int>(utils::getLineIndentation(keyword, sourceManager).size());
                return keyword.getLocWithOffset(indentation + 1 - column);
            }
        };
    } // namespace platform_macros

    using platform_macros::PlatformMacrosCheck;
} // namespace clang::tidy::juce
