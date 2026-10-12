#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace float_max_geometry
    {
        AST_MATCHER(FunctionDecl, isJuceFunction)
        {
            return utils::isInJuceNamespace(Node);
        }

        class FloatMaxGeometryCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto limit = callExpr(callee(cxxMethodDecl(hasAnyName("max", "lowest", "infinity"),
                                                                 ofClass(classTemplateSpecializationDecl(hasName("::std::numeric_limits"),
                                                                                                         hasTemplateArgument(0, refersToType(realFloatingPointType())))))));
                const auto argument = expr(ignoringParenImpCasts(anyOf(limit,
                                                                       unaryOperator(hasOperatorName("-"),
                                                                                     hasUnaryOperand(ignoringParenImpCasts(limit))))))
                                          .bind("argument");

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(callExpr(callee(functionDecl(isJuceFunction()).bind("function")),
                                        hasAnyArgument(argument)),
                               cxxConstructExpr(hasDeclaration(cxxConstructorDecl(isJuceFunction()).bind("function")),
                                                hasAnyArgument(argument)))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* argument = result.Nodes.getNodeAs<Expr>("argument");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");

                if (utils::isInMacroBody(argument->getExprLoc(), *result.SourceManager))
                    return;

                const auto text = utils::getSourceText(argument->getSourceRange(), *result.SourceManager, result.Context->getLangOpts());
                const NamedDecl* callee = function;

                if (const auto* constructor = dyn_cast<CXXConstructorDecl>(function))
                    callee = constructor->getParent();

                diag(argument->getExprLoc(),
                     "'%0' will overflow when it's converted to an integer "
                     "in the implementation of '%1()'. Prefer a large-enough "
                     "finite value like 65,535 (UINT16_MAX)")
                    << text.value_or("std::numeric_limits<float>::max()")
                    << utils::qualifiedName(*callee);
            }
        };
    } // namespace float_max_geometry

    using float_max_geometry::FloatMaxGeometryCheck;
} // namespace clang::tidy::juce
