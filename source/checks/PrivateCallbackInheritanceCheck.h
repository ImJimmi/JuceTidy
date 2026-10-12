#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SetVector.h>

namespace clang::tidy::juce
{
    namespace private_callback_inheritance
    {
        class PrivateCallbackInheritanceCheck : public JuceTidyCheck
        {
        public:
            PrivateCallbackInheritanceCheck(StringRef name, ClangTidyContext* context)
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

                finder->addMatcher(
                    cxxRecordDecl(isDefinition(),
                                  unless(isExpansionInSystemHeader()),
                                  unless(isTemplateInstantiation()),
                                  unless(isLambda()))
                        .bind("record"),
                    this);

                finder->addMatcher(
                    implicitCastExpr(anyOf(hasCastKind(CK_DerivedToBase),
                                           hasCastKind(CK_UncheckedDerivedToBase)),
                                     optionally(forCallable(cxxMethodDecl().bind("callable"))))
                        .bind("cast"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* cast = result.Nodes.getNodeAs<ImplicitCastExpr>("cast"))
                {
                    recordExternalConversion(*cast, result.Nodes.getNodeAs<CXXMethodDecl>("callable"));
                    return;
                }

                sourceManager = result.SourceManager;

                if (const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record"); !utils::isInJuceNamespace(*record))
                    records.insert(record);
            }

            void onEndOfTranslationUnit() override
            {
                for (const auto* record : records)
                {
                    for (const auto& base : record->bases())
                    {
                        const auto* baseRecord = base.getType()->getAsCXXRecordDecl();

                        if (baseRecord == nullptr
                            || base.getAccessSpecifier() != AS_public
                            || base.getBeginLoc().isMacroID()
                            || !isCallbackType(*baseRecord)
                            || convertedExternally.contains({ record->getCanonicalDecl(), baseRecord->getCanonicalDecl() }))
                        {
                            continue;
                        }

                        const auto diagnostic = diag(base.getBeginLoc(),
                                                     "inherit from '%0' privately so its "
                                                     "members don't become part of the "
                                                     "public interface of %1")
                                             << utils::qualifiedName(*baseRecord)
                                             << record;

                        if (base.getAccessSpecifierAsWritten() == AS_none)
                            diagnostic << FixItHint::CreateInsertion(base.getBeginLoc(), "private ");
                        else if (const auto keyword = findPublicKeyword(base); keyword.isValid())
                            diagnostic << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(keyword), "private");
                    }
                }

                records.clear();
                convertedExternally.clear();
            }

        private:
            static inline constexpr auto defaultTypes =
                "::juce::Timer;"
                "::juce::MultiTimer;"
                "::juce::HighResolutionTimer;"
                "::juce::AsyncUpdater;"
                "::juce::ChangeListener;"
                "::juce::ActionListener;"
                "::juce::ComponentListener;"
                "::juce::MouseListener;"
                "::juce::FocusChangeListener;"
                "::juce::AudioProcessorListener;"
                "::juce::MidiInputCallback;"
                "::juce::MidiKeyboardStateListener;"
                "::juce::TimeSliceClient";

            void recordExternalConversion(const ImplicitCastExpr& cast, const CXXMethodDecl* callable)
            {
                if (cast.path_empty())
                    return;

                auto sourceType = cast.getSubExpr()->getType();

                if (sourceType->isPointerType())
                    sourceType = sourceType->getPointeeType();

                const auto* derived = sourceType->getAsCXXRecordDecl();
                const auto* base = (*cast.path_begin())->getType()->getAsCXXRecordDecl();

                if (derived == nullptr || base == nullptr)
                    return;

                if (callable != nullptr && callable->getParent()->getCanonicalDecl() == derived->getCanonicalDecl())
                    return;

                convertedExternally.insert({ derived->getCanonicalDecl(), base->getCanonicalDecl() });
            }

            [[nodiscard]] bool isCallbackType(const CXXRecordDecl& record) const
            {
                const auto name = utils::qualifiedName(record);

                return llvm::is_contained(types, name)
                    || (utils::isInJuceNamespace(record) && StringRef{ name }.ends_with("Listener"));
            }

            [[nodiscard]] SourceLocation findPublicKeyword(const CXXBaseSpecifier& base) const
            {
                const auto location = base.getBeginLoc();
                const auto keyword = Lexer::getSourceText(CharSourceRange::getTokenRange(location), *sourceManager, getLangOpts());

                return keyword == "public" ? location : SourceLocation{};
            }

            const StringRef rawTypes;
            const std::vector<std::string> types;

            const SourceManager* sourceManager = nullptr;
            llvm::SetVector<const CXXRecordDecl*> records;
            llvm::DenseSet<std::pair<const Decl*, const Decl*>> convertedExternally;
        };
    } // namespace private_callback_inheritance

    using private_callback_inheritance::PrivateCallbackInheritanceCheck;
} // namespace clang::tidy::juce
