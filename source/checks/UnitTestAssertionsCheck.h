#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/ADT/StringSwitch.h>

namespace clang::tidy::juce
{
    namespace unit_test_assertions
    {
        class UnitTestAssertionsCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto unitTest = ofClass(isSameOrDerivedFrom(hasName("::juce::UnitTest")));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("expect"),
                                                           unitTest)),
                                      hasArgument(0,
                                                  ignoringParenImpCasts(binaryOperation(hasAnyOperatorName("==", "!=", "<", ">", "<=", ">="),
                                                                                        hasLHS(expr().bind("lhs")),
                                                                                        hasRHS(expr().bind("rhs")))
                                                                            .bind("comparison"))))
                        .bind("expect"),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("expectEquals",
                                                                      "expectNotEquals"),
                                                           unitTest)),
                                      hasArgument(0, hasType(realFloatingPointType())))
                        .bind("floatEquals"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("floatEquals"))
                {
                    if (!call->getExprLoc().isMacroID())
                    {
                        diag(call->getExprLoc(),
                             "use 'expectWithinAbsoluteError()' to compare "
                             "floating-point values");
                    }

                    return;
                }

                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("expect");
                const auto* comparison = result.Nodes.getNodeAs<Expr>("comparison");
                const auto* lhs = result.Nodes.getNodeAs<Expr>("lhs")->IgnoreParenImpCasts();
                const auto* rhs = result.Nodes.getNodeAs<Expr>("rhs")->IgnoreParenImpCasts();

                if (call->getExprLoc().isMacroID())
                    return;

                if (lhs->getType()->isRealFloatingType()
                    || rhs->getType()->isRealFloatingType())
                {
                    diag(call->getExprLoc(),
                         "use 'expectWithinAbsoluteError()' to compare "
                         "floating-point values");

                    return;
                }

                const auto replacement = getReplacement(*comparison);
                const auto diagnostic = diag(call->getExprLoc(),
                                             "use '%0()' instead of comparing "
                                             "values inside 'expect()'")
                                     << replacement;

                const auto lhsText = utils::getSourceText(lhs->getSourceRange(),
                                                          *result.SourceManager,
                                                          getLangOpts());
                const auto rhsText = utils::getSourceText(rhs->getSourceRange(),
                                                          *result.SourceManager,
                                                          getLangOpts());

                if (!lhsText.has_value()
                    || !rhsText.has_value()
                    || lhs->getType().getCanonicalType().getUnqualifiedType() != rhs->getType().getCanonicalType().getUnqualifiedType())
                {
                    return;
                }

                diagnostic << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(call->getExprLoc()),
                                                           replacement)
                           << FixItHint::CreateReplacement(call->getArg(0)->getSourceRange(),
                                                           (*lhsText + ", " + *rhsText).str());
            }

        private:
            [[nodiscard]] static StringRef getReplacement(const Expr& comparison)
            {
                return llvm::StringSwitch<StringRef>(getOperatorName(comparison))
                    .Case("==", "expectEquals")
                    .Case("!=", "expectNotEquals")
                    .Case("<", "expectLessThan")
                    .Case(">", "expectGreaterThan")
                    .Case("<=", "expectLessOrEqual")
                    .Case(">=", "expectGreaterOrEqual")
                    .Default({});
            }

            [[nodiscard]] static StringRef getOperatorName(const Expr& comparison)
            {
                if (const auto* binary = dyn_cast<BinaryOperator>(&comparison))
                    return binary->getOpcodeStr();

                if (const auto* rewritten = dyn_cast<CXXRewrittenBinaryOperator>(&comparison))
                    return rewritten->getOpcodeStr();

                return BinaryOperator::getOpcodeStr(BinaryOperator::getOverloadedOpcode(cast<CXXOperatorCallExpr>(comparison)
                                                                                            .getOperator()));
            }
        };
    } // namespace unit_test_assertions

    using unit_test_assertions::UnitTestAssertionsCheck;
} // namespace clang::tidy::juce
