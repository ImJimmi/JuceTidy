#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang-tidy/utils/LexerUtils.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringExtras.h>

namespace clang::tidy::juce
{
    namespace pass_small_objects_by_value
    {
        class PassSmallObjectsByValueCheck : public JuceTidyCheck
        {
        public:
            PassSmallObjectsByValueCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawTypes{ Options.get("Types", defaultTypes) }
                , types{ utils::parseQualifiedNames(rawTypes) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "Types", rawTypes);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto smallType = namedDecl(hasAnyName(utils::toStringRefs(types)));

                finder->addMatcher(
                    parmVarDecl(hasType(lValueReferenceType(pointee(qualType(isConstQualified(),
                                                                             hasCanonicalType(hasDeclaration(smallType)))))),
                                hasDeclContext(functionDecl(unless(isImplicit()),
                                                            unless(cxxMethodDecl(isOverride())))))
                        .bind("parameter"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* parameter = result.Nodes.getNodeAs<ParmVarDecl>("parameter");
                const auto& context = *result.Context;
                const auto& sourceManager = *result.SourceManager;
                const auto& langOptions = context.getLangOpts();

                if (parameter->getLocation().isMacroID() || parameter->getTypeSourceInfo() == nullptr)
                    return;

                const auto pointeeType = parameter->getType()->getPointeeType().getUnqualifiedType();
                const auto diagnostic = diag(parameter->getBeginLoc(),
                                             "pass %0 by value rather than by "
                                             "ref-to-const")
                                     << pointeeType;

                const auto referenceLoc = parameter->getTypeSourceInfo()->getTypeLoc().getAs<LValueReferenceTypeLoc>();
                const auto* method = dyn_cast<CXXMethodDecl>(parameter->getDeclContext());

                if (!referenceLoc || (method != nullptr && method->isVirtual()))
                    return;

                const auto typeLoc = referenceLoc.getPointeeLoc().getUnqualifiedLoc();
                const auto typeText = utils::getSourceText(typeLoc.getSourceRange(), sourceManager, langOptions);
                const auto constToken = tidy::utils::lexer::getQualifyingToken(tok::kw_const,
                                                                               CharSourceRange::getTokenRange(parameter->getBeginLoc(), referenceLoc.getSigilLoc()),
                                                                               context,
                                                                               sourceManager);

                if (!typeText
                    || !constToken
                    || referenceLoc.getSigilLoc().isMacroID())
                {
                    return;
                }

                const auto start = sourceManager.isBeforeInTranslationUnit(constToken->getLocation(), typeLoc.getBeginLoc())
                                     ? constToken->getLocation()
                                     : typeLoc.getBeginLoc();
                const auto nextCharacter = *sourceManager.getCharacterData(referenceLoc.getSigilLoc().getLocWithOffset(1));
                const auto* const separator = llvm::isAlnum(nextCharacter) || nextCharacter == '_' ? " " : "";

                diagnostic << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(start, referenceLoc.getSigilLoc()),
                                                           typeText->str() + separator);
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            static inline constexpr auto defaultTypes =
                "::juce::Point;"
                "::juce::Rectangle;"
                "::juce::Range;"
                "::juce::Colour;"
                "::juce::Time;"
                "::juce::RelativeTime;"
                "::juce::Identifier;"
                "::juce::ModifierKeys;"
                "::juce::Justification;"
                "::juce::PixelRGB;"
                "::juce::PixelARGB;"
                "::juce::PixelAlpha;"
                "::juce::StringRef;"
                "::juce::CharPointer_ASCII;"
                "::juce::CharPointer_UTF8;"
                "::juce::CharPointer_UTF16;"
                "::juce::CharPointer_UTF32";

            const StringRef rawTypes;
            const std::vector<std::string> types;
        };
    } // namespace pass_small_objects_by_value

    using pass_small_objects_by_value::PassSmallObjectsByValueCheck;
} // namespace clang::tidy::juce
