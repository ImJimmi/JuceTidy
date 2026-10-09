#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/Lex/MacroArgs.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

namespace clang::tidy::juce
{
    namespace prefer_jassert
    {
        class PreferJassertCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                preprocessor->addPPCallbacks(std::make_unique<Callbacks>(*this, sourceManager));
            }

        private:
            class Callbacks : public PPCallbacks
            {
            public:
                Callbacks(ClangTidyCheck& check, const SourceManager& sourceManager)
                    : check{ check }
                    , sourceManager{ sourceManager }
                {
                }

                void MacroExpands(const Token& name,
                                  const MacroDefinition& definition,
                                  SourceRange range,
                                  const MacroArgs* arguments) override
                {
                    const auto location = name.getLocation();
                    const auto* info = definition.getMacroInfo();

                    if (location.isMacroID()
                        || sourceManager.isInSystemHeader(location)
                        || info == nullptr)
                    {
                        return;
                    }

                    const auto macroName = name.getIdentifierInfo()->getName();

                    if (macroName == "assert"
                        && sourceManager.isInSystemHeader(info->getDefinitionLoc()))
                    {
                        check.diag(location,
                                   "use 'jassert()' rather than 'assert()'")
                            << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(location), "jassert");
                    }

                    if (macroName == "jassert"
                        && arguments != nullptr
                        && isAlwaysFalse(*arguments))
                    {
                        check.diag(location,
                                   "use 'jassertfalse' rather than asserting a "
                                   "constant false condition")
                            << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(range), "jassertfalse");
                    }
                }

            private:
                [[nodiscard]] static bool isFalseLiteral(const Token& token)
                {
                    return token.is(tok::kw_false)
                        || (token.is(tok::numeric_constant)
                            && StringRef(token.getLiteralData(), token.getLength()) == "0");
                }

                [[nodiscard]] static bool isAlwaysFalse(const MacroArgs& arguments)
                {
                    const auto* first = arguments.getUnexpArgument(0);

                    return first != nullptr
                        && isFalseLiteral(*first)
                        && first[1].is(tok::eof);
                }

                ClangTidyCheck& check;
                const SourceManager& sourceManager;
            };
        };
    } // namespace prefer_jassert

    using prefer_jassert::PreferJassertCheck;
} // namespace clang::tidy::juce
