#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Preprocessor.h>

#include <string>

namespace clang::tidy::juce
{
    namespace separator_comments
    {
        class SeparatorCommentsCheck : public JuceTidyCheck
        {
        public:
            SeparatorCommentsCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , handler{ *this }
            {
            }

            void registerPPCallbacks(const SourceManager&, Preprocessor* preprocessor, Preprocessor*) override
            {
                preprocessor->addCommentHandler(&handler);
            }

        private:
            class Handler : public CommentHandler
            {
            public:
                explicit Handler(SeparatorCommentsCheck& owner)
                    : check{ owner }
                {
                }

                bool HandleComment(Preprocessor& preprocessor, SourceRange range) override
                {
                    check.handleComment(preprocessor.getSourceManager(), range);
                    return false;
                }

            private:
                SeparatorCommentsCheck& check;
            };

            void handleComment(const SourceManager& sourceManager, SourceRange range)
            {
                if (range.getBegin().isMacroID() || sourceManager.isInSystemHeader(range.getBegin()))
                    return;

                const auto text = Lexer::getSourceText(CharSourceRange::getCharRange(range), sourceManager, getLangOpts());

                if (!isSeparator(text) || isCanonicalSeparator(text) || !isFirstOnLine(range, sourceManager))
                    return;

                const auto diagnostic = reportSeparator(range.getBegin(), text);

                if (text.starts_with("//") || isLastOnLine(range, sourceManager))
                    diagnostic << FixItHint::CreateReplacement(CharSourceRange::getCharRange(range), "//" + std::string(78, '='));
            }

            [[nodiscard]] static bool isSeparator(StringRef text)
            {
                auto body = text;

                if (!body.consume_front("//") && (!body.consume_front("/*") || !body.consume_back("*/")))
                    return false;

                body = body.trim();

                return body.size() >= 3 && body.find_first_not_of("=-*/#~_+") == StringRef::npos;
            }

            [[nodiscard]] static bool isFirstOnLine(SourceRange range, const SourceManager& sourceManager)
            {
                const auto [file, commentBegin] = sourceManager.getDecomposedLoc(range.getBegin());
                const auto preceding = sourceManager.getBufferData(file).substr(0, commentBegin);

                return preceding.substr(preceding.rfind('\n') + 1).trim().empty();
            }

            [[nodiscard]] static bool isLastOnLine(SourceRange range, const SourceManager& sourceManager)
            {
                const auto [file, commentEnd] = sourceManager.getDecomposedLoc(range.getEnd());
                const auto rest = sourceManager.getBufferData(file).substr(commentEnd);

                return rest.take_until([](char character) {
                               return character == '\n';
                           })
                    .trim()
                    .empty();
            }

            DiagnosticBuilder reportSeparator(SourceLocation location,
                                              StringRef text)
            {
                auto body = text;

                if (!body.consume_front("//"))
                {
                    return diag(location,
                                "separator comments should start with '//'");
                }

                if (const auto other = body.find_first_not_of('=');
                    other != StringRef::npos)
                {
                    return diag(location,
                                "separator comments should use '//==...' "
                                "rather than '//%0%0...'")
                        << body.substr(other, 1);
                }

                return diag(location,
                            "separator comments should always be 80 characters "
                            "long regardless of indentation level ('//' "
                            "followed by 78 '=')")
                    << static_cast<unsigned int>(body.size());
            }

            [[nodiscard]] static bool isCanonicalSeparator(StringRef text)
            {
                return text.size() == 80
                    && text.consume_front("//")
                    && text.find_first_not_of('=') == StringRef::npos;
            }

            Handler handler;
        };
    } // namespace separator_comments

    using separator_comments::SeparatorCommentsCheck;
} // namespace clang::tidy::juce
