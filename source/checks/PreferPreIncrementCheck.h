#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace prefer_pre_increment
    {
        AST_MATCHER(UnaryOperator, isPostfixOperator)
        {
            return Node.isPostfix();
        }

        class PreferPreIncrementCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(unaryOperator(isPostfixOperator(),
                                             hasAnyOperatorName("++", "--")),
                               cxxOperatorCallExpr(hasAnyOverloadedOperatorName("++", "--"),
                                                   argumentCountIs(2))))
                        .bind("increment"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* increment = result.Nodes.getNodeAs<Expr>("increment");

                if (!isResultUnused(*increment, *result.Context))
                    return;

                const auto [operatorLocation, operand, isIncrement] = [increment] -> std::tuple<SourceLocation, const Expr*, bool> {
                    if (const auto* unary = dyn_cast<UnaryOperator>(increment))
                    {
                        return {
                            unary->getOperatorLoc(),
                            unary->getSubExpr(),
                            unary->isIncrementOp(),
                        };
                    }

                    const auto* call = cast<CXXOperatorCallExpr>(increment);

                    return {
                        call->getOperatorLoc(),
                        call->getArg(0),
                        call->getOperator() == OO_PlusPlus,
                    };
                }();

                if (operatorLocation.isMacroID() || operand->getBeginLoc().isMacroID())
                    return;

                const auto& sourceManager = *result.SourceManager;
                const auto range = utils::getFileRange(increment->getSourceRange(), sourceManager, getLangOpts());
                const auto operandText = utils::getSourceText(operand->getSourceRange(), sourceManager, getLangOpts());

                const auto diagnostic = diag(operatorLocation,
                                             "use "
                                             "pre-%select{decrement|increment}0 "
                                             "when the result is unused")
                                     << isIncrement;

                if (range && operandText)
                    diagnostic << FixItHint::CreateReplacement(*range, (Twine(isIncrement ? "++" : "--") + *operandText).str());
            }

        private:
            [[nodiscard]] static bool isResultUnused(const Stmt& statement, ASTContext& context)
            {
                const auto singleParent = utils::getSingleParent(statement, context);

                if (!singleParent)
                    return false;

                const auto& parent = *singleParent;
                const auto isChild = [&statement](const Stmt* child) {
                    return child == &statement;
                };

                if (parent.get<CompoundStmt>() != nullptr)
                    return true;

                if (const auto* wrapper = parent.get<ExprWithCleanups>())
                    return isResultUnused(*wrapper, context);

                if (const auto* parens = parent.get<ParenExpr>())
                    return isResultUnused(*parens, context);

                if (const auto* binary = parent.get<BinaryOperator>();
                    binary != nullptr && binary->isCommaOp())
                {
                    return isChild(binary->getLHS()) || isResultUnused(*binary, context);
                }

                if (const auto* forLoop = parent.get<ForStmt>())
                    return isChild(forLoop->getInc()) || isChild(forLoop->getBody());

                if (const auto* ifStatement = parent.get<IfStmt>())
                    return isChild(ifStatement->getThen()) || isChild(ifStatement->getElse());

                if (const auto* whileLoop = parent.get<WhileStmt>())
                    return isChild(whileLoop->getBody());

                if (const auto* doLoop = parent.get<DoStmt>())
                    return isChild(doLoop->getBody());

                return parent.get<SwitchCase>() != nullptr
                    || parent.get<LabelStmt>() != nullptr
                    || parent.get<AttributedStmt>() != nullptr;
            }
        };
    } // namespace prefer_pre_increment

    using prefer_pre_increment::PreferPreIncrementCheck;
} // namespace clang::tidy::juce
