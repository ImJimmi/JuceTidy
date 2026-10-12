#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>

namespace clang::tidy::juce
{
    namespace float_equality
    {
        class FloatEqualityCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    binaryOperator(unless(isInTemplateInstantiation()),
                                   unless(isExpansionInSystemHeader()),
                                   hasAnyOperatorName("==", "!="),
                                   hasLHS(hasType(realFloatingPointType())),
                                   hasRHS(hasType(realFloatingPointType())))
                        .bind("comparison"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* comparison = result.Nodes.getNodeAs<BinaryOperator>("comparison");
                const auto* lhs = comparison->getLHS()->IgnoreParenImpCasts();
                const auto* rhs = comparison->getRHS()->IgnoreParenImpCasts();
                const auto& sourceManager = *result.SourceManager;
                const auto& langOptions = result.Context->getLangOpts();

                if (comparison->getOperatorLoc().isMacroID() || isSameVariable(*lhs, *rhs))
                    return;

                const auto diagnostic = diag(comparison->getOperatorLoc(),
                                             "use 'juce::approximatelyEqual()' "
                                             "to compare floats, or "
                                             "'juce::exactlyEqual()' if you "
                                             "really do need a bit-for-bit "
                                             "comparison (which is unlikely)")
                                     << comparison->getOpcodeStr();

                const auto lhsText = utils::getSourceText(lhs->getSourceRange(),
                                                          sourceManager,
                                                          langOptions);
                const auto rhsText = utils::getSourceText(rhs->getSourceRange(),
                                                          sourceManager,
                                                          langOptions);

                if (!lhsText.has_value()
                    || !rhsText.has_value()
                    || lhs->getType().getCanonicalType().getUnqualifiedType() != rhs->getType().getCanonicalType().getUnqualifiedType())
                {
                    return;
                }

                const auto replacement = std::string{ comparison->getOpcode() == BO_NE ? "!" : "" }
                                       + "juce::approximatelyEqual(" + lhsText->str() + ", " + rhsText->str() + ")";

                diagnostic << FixItHint::CreateReplacement(comparison->getSourceRange(), replacement);
            }

        private:
            [[nodiscard]] static bool isSameVariable(const Expr& lhs, const Expr& rhs)
            {
                const auto* lhsReference = dyn_cast<DeclRefExpr>(&lhs);
                const auto* rhsReference = dyn_cast<DeclRefExpr>(&rhs);

                return lhsReference != nullptr && rhsReference != nullptr && lhsReference->getDecl() == rhsReference->getDecl();
            }
        };
    } // namespace float_equality

    using float_equality::FloatEqualityCheck;
} // namespace clang::tidy::juce
