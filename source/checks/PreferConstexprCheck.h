#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang-tidy/utils/FileExtensionsUtils.h>
#include <clang-tidy/utils/LexerUtils.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace prefer_constexpr
    {
        class PreferConstexprCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    varDecl(hasType(isConstQualified()),
                            unless(isConstexpr()),
                            unless(parmVarDecl()),
                            unless(decompositionDecl()),
                            unless(isInstantiated()),
                            hasInitializer(expr()),
                            optionally(allOf(hasAutomaticStorageDuration(),
                                             hasParent(declStmt(hasParent(compoundStmt().bind("scope")))))))
                        .bind("variable"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* variable = result.Nodes.getNodeAs<VarDecl>("variable");
                const auto* scope = result.Nodes.getNodeAs<CompoundStmt>("scope");
                auto& context = *result.Context;

                if (!isCandidate(*variable, context))
                    return;

                const auto specifier = getStorageSpecifier(*variable, scope, getHeaderFileExtensions(), context);

                const auto diagnostic = diag(variable->getLocation(),
                                             "%0 should be declared "
                                             "'%1constexpr'")
                                     << variable
                                     << specifier;

                if (const auto fix = makeFix(*variable, specifier, context))
                    diagnostic << *fix;
            }

        private:
            [[nodiscard]] static bool isCandidate(const VarDecl& variable, const ASTContext& context)
            {
                const auto type = variable.getType();

                if (variable.isInitCapture()
                    || variable.hasExternalStorage()
                    || variable.isCXXForRangeDecl()
                    || variable.getTSCSpec() != TSCS_unspecified
                    || variable.getLocation().isMacroID()
                    || variable.getDeclContext()->isDependentContext())
                {
                    return false;
                }

                if (type->isReferenceType()
                    || type.isVolatileQualified()
                    || type->isDependentType()
                    || !type->isLiteralType(context))
                {
                    return false;
                }

                if (const auto* record = type->getAsCXXRecordDecl();
                    record != nullptr && record->isLambda())
                {
                    return false;
                }

                const auto* initialiser = variable.getInit();

                return !initialiser->isValueDependent()
                    && !initialiser->isTypeDependent()
                    && initialiser->isCXX11ConstantExpr(context);
            }

            [[nodiscard]] static bool canBeStatic(const VarDecl& variable, const CompoundStmt& scope, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto* function = dyn_cast<FunctionDecl>(variable.getDeclContext());

                if (function == nullptr
                    || (function->isConstexpr() && !context.getLangOpts().CPlusPlus23))
                {
                    return false;
                }

                const auto explicitCapture = lambdaExpr(hasAnyCapture(lambdaCapture(unless(isImplicit()),
                                                                                    capturesVar(equalsNode(&variable)))));
                return utils::findFirst(explicitCapture, scope, context) == nullptr;
            }

            [[nodiscard]] static bool shouldBeInline(const VarDecl& variable,
                                                     const FileExtensionsSet& headerExtensions,
                                                     const ASTContext& context)
            {
                if (!context.getLangOpts().CPlusPlus17
                    || !variable.getDeclContext()->getRedeclContext()->isFileContext())
                {
                    return false;
                }

                if (variable.getStorageClass() != SC_None
                    || variable.isInline()
                    || variable.isInAnonymousNamespace())
                {
                    return false;
                }

                return tidy::utils::isExpansionLocInHeaderFile(variable.getLocation(), context.getSourceManager(), headerExtensions);
            }

            [[nodiscard]] static llvm::StringRef getStorageSpecifier(const VarDecl& variable,
                                                                     const CompoundStmt* scope,
                                                                     const FileExtensionsSet& headerExtensions,
                                                                     ASTContext& context)
            {
                if (scope != nullptr)
                    return canBeStatic(variable, *scope, context) ? "static " : "";

                return shouldBeInline(variable, headerExtensions, context) ? "inline " : "";
            }

            [[nodiscard]] static SourceLocation getTypeStart(SourceLocation location,
                                                             const SourceManager& sourceManager,
                                                             const LangOptions& langOptions)
            {
                const auto previous = tidy::utils::lexer::getPreviousToken(location, sourceManager, langOptions);

                return previous && utils::isRawIdentifier(*previous, "const")
                         ? getTypeStart(previous->getLocation(), sourceManager, langOptions)
                         : location;
            }

            [[nodiscard]] static std::optional<FixItHint> makeFix(const VarDecl& variable,
                                                                  llvm::StringRef specifier,
                                                                  ASTContext& context)
            {
                const auto& sourceManager = context.getSourceManager();
                const auto& langOptions = context.getLangOpts();
                const auto* typeInfo = variable.getTypeSourceInfo();

                const auto next = Lexer::findNextToken(variable.getEndLoc(), sourceManager, langOptions);
                const auto previous = tidy::utils::lexer::getPreviousToken(variable.getLocation(), sourceManager, langOptions);

                if (typeInfo == nullptr
                    || !next
                    || next->is(tok::comma)
                    || (previous && previous->is(tok::comma)))
                {
                    return std::nullopt;
                }

                const auto constToken = tidy::utils::lexer::getQualifyingToken(tok::kw_const,
                                                                               CharSourceRange::getCharRange(variable.getBeginLoc(), variable.getLocation()),
                                                                               context,
                                                                               sourceManager);

                if (!constToken || constToken->getLocation().isMacroID())
                    return std::nullopt;

                const auto keyword = (specifier + "constexpr").str();
                const auto typeLocation = sourceManager.getExpansionLoc(typeInfo->getTypeLoc().getBeginLoc());

                if (sourceManager.isBeforeInTranslationUnit(constToken->getLocation(), typeLocation))
                    return FixItHint::CreateReplacement(CharSourceRange::getTokenRange(constToken->getLocation()), keyword);

                const auto typeStart = getTypeStart(typeLocation, sourceManager, langOptions);
                const auto afterConst = Lexer::findNextToken(constToken->getLocation(), sourceManager, langOptions);

                if (!afterConst)
                    return std::nullopt;

                const auto type = Lexer::getSourceText(CharSourceRange::getCharRange(typeStart, constToken->getLocation()),
                                                       sourceManager,
                                                       langOptions);
                return FixItHint::CreateReplacement(CharSourceRange::getCharRange(typeStart, afterConst->getLocation()),
                                                    keyword + " " + type.str());
            }
        };
    } // namespace prefer_constexpr

    using prefer_constexpr::PreferConstexprCheck;
} // namespace clang::tidy::juce
