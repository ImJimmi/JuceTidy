#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace valuetree_identity_comparison
    {
        class ValuetreeIdentityComparisonCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto valueTree = cxxRecordDecl(hasName("::juce::ValueTree"));
                const auto newTree = cxxConstructExpr(hasType(valueTree),
                                                      argumentCountIs(1),
                                                      unless(hasDeclaration(cxxConstructorDecl(anyOf(isCopyConstructor(),
                                                                                                     isMoveConstructor())))));
                const auto freshTree = ignoringImplicit(expr(anyOf(cxxMemberCallExpr(callee(cxxMethodDecl(hasName("createCopy"),
                                                                                                          ofClass(valueTree)))),
                                                                   callExpr(callee(functionDecl(hasAnyName("::juce::ValueTree::fromXml",
                                                                                                           "::juce::ValueTree::readFromData",
                                                                                                           "::juce::ValueTree::readFromStream",
                                                                                                           "::juce::ValueTree::readFromGZIPData")))),
                                                                   newTree,
                                                                   cxxFunctionalCastExpr(hasType(valueTree),
                                                                                         hasDescendant(newTree)))));

                finder->addMatcher(
                    cxxOperatorCallExpr(unless(isInTemplateInstantiation()),
                                        hasAnyOverloadedOperatorName("==", "!="),
                                        callee(cxxMethodDecl(ofClass(valueTree))),
                                        anyOf(hasArgument(0, freshTree),
                                              hasArgument(1, freshTree)))
                        .bind("comparison"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* comparison = result.Nodes.getNodeAs<CXXOperatorCallExpr>("comparison");
                const auto& sourceManager = *result.SourceManager;
                const auto& langOptions = result.Context->getLangOpts();

                if (utils::isInMacroBody(comparison->getOperatorLoc(), sourceManager))
                    return;

                const auto isInequality = comparison->getOperator() == OO_ExclaimEqual;
                const auto diagnostic = diag(comparison->getOperatorLoc(),
                                             "'%select{==|!=}0' checks that "
                                             "the two ValueTrees use the same "
                                             "underlying object. Use "
                                             "'isEquivalentTo()' to compare "
                                             "their properties and children")
                                     << isInequality;

                const auto* left = comparison->getArg(0);
                const auto range = utils::getFileRange(comparison->getSourceRange(), sourceManager, langOptions);
                const auto leftText = utils::getSourceText(left->getSourceRange(), sourceManager, langOptions);
                const auto rightText = utils::getSourceText(comparison->getArg(1)->getSourceRange(), sourceManager, langOptions);

                if (!range
                    || !leftText
                    || !rightText
                    || !isPostfixExpression(*left))
                {
                    return;
                }

                const auto* const negation = isInequality ? "!" : "";
                diagnostic << FixItHint::CreateReplacement(*range,
                                                           (Twine(negation) + *leftText + ".isEquivalentTo(" + *rightText + ")").str());
            }

        private:
            [[nodiscard]] static bool isPostfixExpression(const Expr& expression)
            {
                return isa<DeclRefExpr,
                           MemberExpr,
                           CallExpr,
                           ParenExpr>(expression.IgnoreImplicit());
            }
        };
    } // namespace valuetree_identity_comparison

    using valuetree_identity_comparison::ValuetreeIdentityComparisonCheck;
} // namespace clang::tidy::juce
