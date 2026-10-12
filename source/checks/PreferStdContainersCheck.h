#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang-tidy/utils/IncludeInserter.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringSwitch.h>

namespace clang::tidy::juce
{
    namespace prefer_std_containers
    {
        class PreferStdContainersCheck : public JuceTidyCheck
        {
        public:
            PreferStdContainersCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawIgnoredTypes{ Options.get("IgnoredTypes", "") }
                , ignoredTypes{ utils::parseQualifiedNames(rawIgnoredTypes) }
                , includeInserter{ Options.getLocalOrGlobal("IncludeStyle", tidy::utils::IncludeSorter::IS_LLVM), areDiagsSelfContained() }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "IgnoredTypes", rawIgnoredTypes);
                Options.store(options, "IncludeStyle", includeInserter.getStyle());
            }

            void registerPPCallbacks(const SourceManager&, Preprocessor* preprocessor, Preprocessor*) override
            {
                includeInserter.registerPreprocessor(preprocessor);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto overriding = cxxMethodDecl(isOverride());

                finder->addMatcher(
                    declaratorDecl(unless(isImplicit()),
                                   unless(isExpansionInSystemHeader()),
                                   unless(isInstantiated()),
                                   anyOf(varDecl(),
                                         fieldDecl(),
                                         functionDecl(unless(overriding),
                                                      unless(cxxConversionDecl()),
                                                      unless(hasParent(cxxRecordDecl(isLambda()))))))
                        .bind("declaration"),
                    this);

                finder->addMatcher(
                    typeLoc(loc(typedefType(hasDeclaration(typedefNameDecl(hasAnyName("::juce::int8",
                                                                                      "::juce::uint8",
                                                                                      "::juce::int16",
                                                                                      "::juce::uint16",
                                                                                      "::juce::int32",
                                                                                      "::juce::uint32",
                                                                                      "::juce::int64",
                                                                                      "::juce::uint64",
                                                                                      "::juce::pointer_sized_int",
                                                                                      "::juce::pointer_sized_uint"))
                                                               .bind("alias")))))
                        .bind("aliasLoc"),
                    this);
            }

            void onStartOfTranslationUnit() override
            {
                reportedAliases.clear();
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* aliasLoc = result.Nodes.getNodeAs<TypeLoc>("aliasLoc"))
                {
                    reportIntegerAlias(*aliasLoc, *result.Nodes.getNodeAs<TypedefNameDecl>("alias"), *result.SourceManager);
                    return;
                }

                const auto* declaration = result.Nodes.getNodeAs<DeclaratorDecl>("declaration");
                const auto* function = dyn_cast<FunctionDecl>(declaration);
                const auto type = function != nullptr ? function->getReturnType() : declaration->getType();

                if (type->getContainedAutoType() != nullptr
                    || declaration->getTypeSpecStartLoc().isMacroID()
                    || isParameterOfFixedSignature(*declaration))
                {
                    return;
                }

                const auto* record = getRecord(type);

                if (record == nullptr)
                    return;

                const auto name = utils::qualifiedName(*record);
                const auto replacement = getReplacement(name);

                if (replacement.empty() || llvm::is_contained(ignoredTypes, name))
                    return;

                if (name == "juce::MemoryBlock" && isView(type))
                {
                    const auto isConst = getPointee(type).isConstQualified();

                    diag(declaration->getTypeSpecStartLoc(),
                         "use '%select{std::span<std::byte>|std::span<const std::byte>}0' "
                         "instead of 'juce::MemoryBlock' to refer to data owned "
                         "elsewhere%select{, or 'std::vector<std::byte>&' if "
                         "the data is resized|}0")
                        << isConst;

                    return;
                }

                diag(declaration->getTypeSpecStartLoc(),
                     "use '%0' instead of '%1'")
                    << replacement
                    << name;
            }

        private:
            void reportIntegerAlias(const TypeLoc& aliasLoc, const TypedefNameDecl& alias, const SourceManager& sourceManager)
            {
                const auto name = utils::qualifiedName(alias);
                const auto location = aliasLoc.getBeginLoc();

                if (location.isMacroID()
                    || sourceManager.isInSystemHeader(location)
                    || llvm::is_contained(ignoredTypes, name)
                    || !reportedAliases.insert(location).second)
                {
                    return;
                }

                const auto replacement = getIntegerReplacement(alias.getName());
                const auto diagnostic = diag(location,
                                             "use '%0' instead of '%1'")
                                     << replacement
                                     << name
                                     << FixItHint::CreateReplacement(aliasLoc.getSourceRange(), replacement);

                if (auto include = includeInserter.createIncludeInsertion(sourceManager.getFileID(sourceManager.getExpansionLoc(location)), "<cstdint>"))
                    diagnostic << *include;
            }

            [[nodiscard]] static std::string getIntegerReplacement(StringRef juceAlias)
            {
                if (juceAlias == "pointer_sized_int")
                    return "std::intptr_t";

                if (juceAlias == "pointer_sized_uint")
                    return "std::uintptr_t";

                return "std::" + juceAlias.str() + "_t";
            }

            [[nodiscard]] static QualType getPointee(QualType type)
            {
                type = type.getNonReferenceType();
                return type->isPointerType() ? type->getPointeeType() : type;
            }

            [[nodiscard]] static bool isView(QualType type)
            {
                return type->isReferenceType() || type->isPointerType();
            }

            [[nodiscard]] static bool isParameterOfFixedSignature(const DeclaratorDecl& declaration)
            {
                if (!isa<ParmVarDecl>(declaration))
                    return false;

                const auto* method = dyn_cast<CXXMethodDecl>(declaration.getDeclContext());

                return method != nullptr && (method->size_overridden_methods() > 0 || method->getParent()->isLambda());
            }

            [[nodiscard]] static const CXXRecordDecl* getRecord(QualType type)
            {
                type = type.getNonReferenceType();

                while (type->isPointerType())
                    type = type->getPointeeType();

                const auto* record = type->getAsCXXRecordDecl();

                if (const auto* specialisation = dyn_cast_or_null<ClassTemplateSpecializationDecl>(record))
                    return specialisation->getSpecializedTemplate()->getTemplatedDecl();

                return record;
            }

            [[nodiscard]] static StringRef getReplacement(StringRef juceType)
            {
                return llvm::StringSwitch<StringRef>(juceType)
                    .Case("juce::Array", "std::vector")
                    .Case("juce::OwnedArray", "std::vector<std::unique_ptr>")
                    .Case("juce::HashMap", "std::unordered_map")
                    .Case("juce::SortedSet", "std::set")
                    .Case("juce::HeapBlock", "std::vector")
                    .Case("juce::ScopedPointer", "std::unique_ptr")
                    .Case("juce::CriticalSection", "std::recursive_mutex")
                    .Case("juce::GenericScopedLock", "std::scoped_lock")
                    .Case("juce::Span", "std::span")
                    .Case("juce::Optional", "std::optional")
                    .Case("juce::Atomic", "std::atomic")
                    .Case("juce::MemoryBlock", "std::vector<std::byte>")
                    .Default({});
            }

            const StringRef rawIgnoredTypes;
            const std::vector<std::string> ignoredTypes;

            tidy::utils::IncludeInserter includeInserter;
            llvm::DenseSet<SourceLocation> reportedAliases;
        };
    } // namespace prefer_std_containers

    using prefer_std_containers::PreferStdContainersCheck;
} // namespace clang::tidy::juce
