#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Lex/Preprocessor.h>

namespace clang::tidy::juce
{
    namespace leak_detector
    {
        class LeakDetectorCheck : public JuceTidyCheck
        {
        public:
            LeakDetectorCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawBaseClasses{ Options.get("BaseClasses", defaultBaseClasses) }
                , baseClasses{ utils::parseQualifiedNames(rawBaseClasses) }
                , rawMacros{ Options.get("Macros", defaultMacros) }
                , macros{ utils::parseQualifiedNames(rawMacros) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "BaseClasses", rawBaseClasses);
                Options.store(options, "Macros", rawMacros);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxRecordDecl(isDefinition(),
                                  unless(isLambda()),
                                  unless(isUnion()),
                                  utils::isSameOrDerivedFromAnyOf(baseClasses))
                        .bind("record"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                const auto& sourceManager = *result.SourceManager;

                if (record->isLocalClass() != nullptr
                    || record->getIdentifier() == nullptr
                    || record->getLocation().isMacroID())
                {
                    return;
                }

                if (llvm::any_of(record->fields(),
                                 [](const FieldDecl* field) {
                                     return isLeakDetector(field->getType());
                                 }))
                {
                    return;
                }

                const auto braces = record->getBraceRange();
                const auto name = record->getName();
                const auto hasLeakDetectorMacro = llvm::any_of(expansions,
                                                               [&](const Expansion& expansion) {
                                                                   return expansion.className == name
                                                                       && sourceManager.isPointWithin(expansion.location,
                                                                                                      sourceManager.getExpansionLoc(braces.getBegin()),
                                                                                                      sourceManager.getExpansionLoc(braces.getEnd()));
                                                               });

                if (hasLeakDetectorMacro)
                    return;

                diag(record->getLocation(),
                     "'%0' has no leak detector. Add 'JUCE_LEAK_DETECTOR(%0)' "
                     "somewhere in the class")
                    << record->getNameAsString();
            }

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                expansions.clear();

                preprocessor->addPPCallbacks(std::make_unique<utils::MacroExpansionListener>(
                    macros,
                    [this, &sourceManager](const Token& name, SourceRange, const MacroArgs* arguments) {
                        const auto* argument = arguments != nullptr
                                                 ? arguments->getUnexpArgument(0)
                                                 : nullptr;

                        if (argument != nullptr && argument->is(tok::identifier))
                        {
                            expansions.push_back({
                                .location = sourceManager.getExpansionLoc(name.getLocation()),
                                .className = argument->getIdentifierInfo()->getName().str(),
                            });
                        }
                    }));
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            struct Expansion
            {
                SourceLocation location;
                std::string className;
            };

            static inline constexpr auto defaultBaseClasses =
                "::juce::Component;"
                "::juce::AudioProcessor";
            static inline constexpr auto defaultMacros =
                "JUCE_LEAK_DETECTOR;"
                "JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR";

            [[nodiscard]] static bool isLeakDetector(QualType type)
            {
                static const std::vector<std::string> detector{ "juce::LeakedObjectDetector" };

                if (const auto* specialisation = type->getAs<TemplateSpecializationType>())
                {
                    if (const auto* templateDecl = specialisation->getTemplateName().getAsTemplateDecl())
                        return utils::qualifiedName(*templateDecl) == detector.front();
                }

                return utils::isSameOrDerivedFrom(type->getAsCXXRecordDecl(), detector);
            }

            const StringRef rawBaseClasses;
            const std::vector<std::string> baseClasses;
            const StringRef rawMacros;
            const std::vector<std::string> macros;
            std::vector<Expansion> expansions;
        };
    } // namespace leak_detector

    using leak_detector::LeakDetectorCheck;
} // namespace clang::tidy::juce
