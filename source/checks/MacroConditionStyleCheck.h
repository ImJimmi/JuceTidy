#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/Regex.h>

namespace clang::tidy::juce
{
    namespace macro_condition_style
    {
        class MacroConditionStyleCheck : public JuceTidyCheck
        {
        public:
            MacroConditionStyleCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawFlagMacros{ Options.get("FlagMacros", defaultFlagMacros) }
                , flagMacros{ rawFlagMacros }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "FlagMacros", rawFlagMacros);
            }

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
            static inline constexpr auto defaultFlagMacros = "^(JUCE|JucePlugin)_";

            void checkDirective(const utils::ConditionalDirective& directive,
                                Preprocessor& preprocessor,
                                const SourceManager& sourceManager)
            {
                const auto& tokens = directive.tokens;
                const auto isUncheckedFlag = [&](StringRef name) {
                    return isFlag(name, preprocessor)
                        && !utils::isFollowedByDefinitionOf(directive, name, sourceManager);
                };
                const auto reportIgnoredValue = [this](const Token& name,
                                                       CharSourceRange range,
                                                       StringRef original,
                                                       StringRef suggestion,
                                                       StringRef replacement) {
                    diag(name.getLocation(),
                         "'%0' is always defined, but set to 0 when it's not "
                         "enabled. Use '%1' instead")
                        << utils::getRawSpelling(name)
                        << suggestion
                        << FixItHint::CreateReplacement(range, replacement);
                };

                if (directive.isDefinedCheck())
                {
                    const auto name = utils::getRawSpelling(tokens.at(1));

                    if (!isUncheckedFlag(name))
                        return;

                    const auto* const keyword = directive.keyword().starts_with("el") ? "elif" : "if";
                    const auto* const negation = directive.keyword().ends_with("ndef") ? "!" : "";
                    const auto replacement = (keyword + Twine(" ") + negation + name).str();
                    reportIgnoredValue(tokens.at(1),
                                       utils::getTokensRange(tokens.at(0), tokens.at(1)),
                                       ("#" + directive.keyword() + " " + name).str(),
                                       "#" + replacement,
                                       replacement);
                    return;
                }

                for (std::size_t index = 1; index < tokens.size(); ++index)
                {
                    if (const auto defined = utils::matchDefinedOperator(tokens, index))
                    {
                        const auto& name = tokens.at(defined->name);
                        const auto range = utils::getTokensRange(tokens.at(index), tokens.at(defined->last));

                        if (isUncheckedFlag(utils::getRawSpelling(name)))
                        {
                            reportIgnoredValue(name,
                                               range,
                                               Lexer::getSourceText(range, sourceManager, getLangOpts()),
                                               utils::getRawSpelling(name),
                                               utils::getRawSpelling(name));
                        }

                        index = defined->last;
                        continue;
                    }

                    if (index + 2 >= tokens.size()
                        || !tokens.at(index + 1).isOneOf(tok::equalequal, tok::exclaimequal))
                    {
                        continue;
                    }

                    const auto& left = tokens.at(index);
                    const auto& right = tokens.at(index + 2);
                    const auto* flag = getComparedFlag(left, right);

                    if (flag == nullptr || !isFlag(utils::getRawSpelling(*flag), preprocessor))
                        continue;

                    const auto& value = flag == &left ? right : left;
                    const auto isSet = tokens.at(index + 1).is(tok::equalequal) == (utils::getRawSpelling(value) == "1");
                    const auto replacement = ((isSet ? "" : "!") + utils::getRawSpelling(*flag)).str();
                    const auto range = utils::getTokensRange(left, right);

                    diag(flag->getLocation(),
                         "treat '%0' as a boolean by removing the superfluous "
                         "comparison here")
                        << utils::getRawSpelling(*flag)
                        << Lexer::getSourceText(range, sourceManager, getLangOpts())
                        << FixItHint::CreateReplacement(range, replacement);

                    index += 2;
                }
            }

            [[nodiscard]] bool isFlag(StringRef macroName, Preprocessor& preprocessor) const
            {
                if (!flagMacros.match(macroName))
                    return false;

                const auto* info = preprocessor.getMacroInfo(preprocessor.getIdentifierInfo(macroName));

                if (info == nullptr)
                    return isOnlyDefinedWhenSet(macroName);

                if (info->isFunctionLike()
                    || info->getNumTokens() != 1
                    || info->getReplacementToken(0).isNot(tok::numeric_constant))
                {
                    return false;
                }

                const auto value = preprocessor.getSpelling(info->getReplacementToken(0));
                return value == "0" || value == "1";
            }

            [[nodiscard]] static bool isOnlyDefinedWhenSet(StringRef macroName)
            {
                static const llvm::Regex juceTargetMacros("^JUCE_(WINDOWS|MAC|IOS|IPHONE|LINUX|ANDROID|BSD|WASM|MINGW|DEBUG|64BIT|32BIT|ARM|INTEL|LITTLE_ENDIAN|BIG_ENDIAN|"
                                                          "CLANG|GCC|MSVC|MODULE_AVAILABLE_[A-Za-z0-9_]+)$");
                return juceTargetMacros.match(macroName);
            }

            [[nodiscard]] static bool isZeroOrOne(const Token& token)
            {
                const auto spelling = utils::getRawSpelling(token);
                return token.is(tok::numeric_constant) && (spelling == "0" || spelling == "1");
            }

            [[nodiscard]] static const Token* getComparedFlag(const Token& left, const Token& right)
            {
                if (left.is(tok::raw_identifier) && isZeroOrOne(right))
                    return &left;

                if (right.is(tok::raw_identifier) && isZeroOrOne(left))
                    return &right;

                return nullptr;
            }

            const StringRef rawFlagMacros;
            const llvm::Regex flagMacros;
        };
    } // namespace macro_condition_style

    using macro_condition_style::MacroConditionStyleCheck;
} // namespace clang::tidy::juce
