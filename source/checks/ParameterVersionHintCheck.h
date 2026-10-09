#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace parameter_version_hint
    {
        class ParameterVersionHintCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto parameterClass = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorParameterWithID")));
                const auto recordNamed = [](auto... names) {
                    return hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(hasAnyName(names...))))));
                };
                const auto idLike = expr(anyOf(recordNamed("::juce::ParameterID",
                                                           "::juce::String",
                                                           "::juce::StringRef",
                                                           "::juce::Identifier",
                                                           "::std::basic_string",
                                                           "::std::basic_string_view"),
                                               hasType(arrayType()),
                                               hasType(pointerType())));

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(parameterClass),
                                                                       hasParameter(0, hasType(references(cxxRecordDecl(hasName("::juce::ParameterID"))))))),
                                     hasArgument(0, expr().bind("id"))),
                    this);

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(hasAnyName("::std::make_unique",
                                                            "::std::make_shared"),
                                                 hasTemplateArgument(0,
                                                                     refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(parameterClass))))))),
                             hasArgument(0, idLike.bind("id"))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* id = result.Nodes.getNodeAs<Expr>("id");
                const auto& context = *result.Context;
                const auto& sourceManager = *result.SourceManager;
                const auto* construction = findParameterIdConstruction(*id);

                if (construction != nullptr
                    && (construction->getConstructor()->isCopyOrMoveConstructor()
                        || hasVersionHint(*construction, context)))
                {
                    return;
                }

                const auto isParameterIdObject = construction == nullptr
                                              && id->getType()->getAsCXXRecordDecl() != nullptr
                                              && utils::qualifiedName(*id->getType()->getAsCXXRecordDecl()) == "juce::ParameterID";

                if (isParameterIdObject || utils::isInMacroBody(id->getBeginLoc(), sourceManager))
                    return;

                const auto diagnostic = diag(id->getBeginLoc(),
                                             "parameter ID has no version "
                                             "hint. Use 'juce::ParameterID{ "
                                             "id, versionHint }' with "
                                             "versionHint > 0");
                const auto isConvertedString = construction == nullptr
                                            || (construction->getNumArgs() > 0
                                                && construction->getArg(0)->getBeginLoc() == id->getBeginLoc());

                if (isConvertedString)
                {
                    const auto range = utils::getFileRange(id->getSourceRange(), sourceManager, context.getLangOpts());

                    if (range)
                    {
                        diagnostic << FixItHint::CreateReplacement(*range,
                                                                   "juce::ParameterID{ "
                                                                       + Lexer::getSourceText(*range, sourceManager, context.getLangOpts()).str()
                                                                       + ", 1 }");
                    }

                    return;
                }

                if (construction->getNumArgs() < 2)
                    return;

                const auto* version = construction->getArg(1);

                if (isa<CXXDefaultArgExpr>(version))
                {
                    const auto end = Lexer::getLocForEndOfToken(construction->getArg(0)->getEndLoc(), 0, sourceManager, context.getLangOpts());

                    if (end.isValid() && !end.isMacroID())
                        diagnostic << FixItHint::CreateInsertion(end, ", 1");

                    return;
                }

                if (const auto range = utils::getFileRange(version->getSourceRange(), sourceManager, context.getLangOpts()))
                    diagnostic << FixItHint::CreateReplacement(*range, "1");
            }

        private:
            [[nodiscard]] static const CXXConstructExpr* findParameterIdConstruction(const Expr& expression)
            {
                const auto* current = expression.IgnoreImplicit();

                while (const auto* cast = dyn_cast<CXXFunctionalCastExpr>(current))
                    current = cast->getSubExpr()->IgnoreImplicit();

                const auto* construction = dyn_cast<CXXConstructExpr>(current);

                if (construction == nullptr
                    || utils::qualifiedName(*construction->getConstructor()->getParent()) != "juce::ParameterID")
                {
                    return nullptr;
                }

                return construction;
            }

            [[nodiscard]] static bool hasVersionHint(const CXXConstructExpr& construction, const ASTContext& context)
            {
                if (construction.getNumArgs() < 2 || isa<CXXDefaultArgExpr>(construction.getArg(1)))
                    return false;

                const auto version = construction.getArg(1)->getIntegerConstantExpr(context);
                return !version || *version != 0;
            }
        };
    } // namespace parameter_version_hint

    using parameter_version_hint::ParameterVersionHintCheck;
} // namespace clang::tidy::juce
