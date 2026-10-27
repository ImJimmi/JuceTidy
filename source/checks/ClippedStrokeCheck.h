#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>

namespace clang::tidy::juce
{
    namespace clipped_stroke
    {
        class ClippedStrokeCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto graphicsMethod = [](auto... names) {
                    return callee(cxxMethodDecl(hasAnyName(names...), ofClass(hasName("::juce::Graphics"))));
                };
                const auto inBody = forCallable(functionDecl(hasBody(stmt().bind("body"))));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      graphicsMethod("drawRoundedRectangle", "drawEllipse"),
                                      hasArgument(0, expr().bind("bounds")),
                                      inBody)
                        .bind("call"),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      graphicsMethod("strokePath"),
                                      hasArgument(0, ignoringParenImpCasts(declRefExpr(to(varDecl(hasLocalStorage(), hasType(cxxRecordDecl(hasName("::juce::Path")))).bind("path"))))),
                                      hasArgument(2, cxxDefaultArgExpr()),
                                      inBody)
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto& body = *result.Nodes.getNodeAs<Stmt>("body");
                auto& context = *result.Context;

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                ExprMutationAnalyzer analyzer{ body, context };

                if (const auto* bounds = result.Nodes.getNodeAs<Expr>("bounds"))
                {
                    if (isUnreducedBounds(*bounds, analyzer))
                    {
                        diag(call->getExprLoc(),
                             "'%0()' centres the line on the perimeter of the "
                             "component's bounds, so half of it will be "
                             "clipped. Reduce the bounds by half the line "
                             "thickness")
                            << utils::getMethodName(*call);
                    }

                    return;
                }

                const auto& path = *result.Nodes.getNodeAs<VarDecl>("path");

                if (const auto* add = findUnreducedBoundsAdd(path, body, analyzer, context))
                {
                    diag(call->getExprLoc(),
                         "'strokePath()' centres the stroke on the outline of "
                         "%0, which is built from the component's bounds, so "
                         "half of the stroke will be clipped. Reduce the "
                         "bounds by half the stroke width")
                        << &path;

                    diag(add->getExprLoc(),
                         "%0 is built from the component's bounds here",
                         DiagnosticIDs::Note)
                        << &path;
                }
            }

        private:
            [[nodiscard]] static bool isMethodOf(const CXXMemberCallExpr& call, StringRef method, StringRef record)
            {
                return utils::getMethodName(call) == method
                    && utils::qualifiedName(*call.getMethodDecl()->getParent()) == record;
            }

            [[nodiscard]] static bool isUnreducedBounds(const Expr& expression, ExprMutationAnalyzer& analyzer)
            {
                const auto* bounds = expression.IgnoreUnlessSpelledInSource();

                if (const auto* call = dyn_cast<CXXMemberCallExpr>(bounds))
                {
                    if (isMethodOf(*call, "getLocalBounds", "juce::Component"))
                        return true;

                    return isMethodOf(*call, "toFloat", "juce::Rectangle")
                        && isUnreducedBounds(*call->getImplicitObjectArgument(), analyzer);
                }

                const auto* reference = dyn_cast<DeclRefExpr>(bounds);
                const auto* variable = reference != nullptr ? dyn_cast<VarDecl>(reference->getDecl()) : nullptr;

                return variable != nullptr
                    && variable->hasLocalStorage()
                    && !variable->getType()->isReferenceType()
                    && variable->hasInit()
                    && isUnreducedBounds(*variable->getInit(), analyzer)
                    && !analyzer.isMutated(variable);
            }

            [[nodiscard]] static const CXXMemberCallExpr* findUnreducedBoundsAdd(const VarDecl& path,
                                                                                 const Stmt& body,
                                                                                 ExprMutationAnalyzer& analyzer,
                                                                                 ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto onPath = [&](auto... names) {
                    return cxxMemberCallExpr(on(ignoringParenImpCasts(declRefExpr(to(equalsNode(&path))))),
                                             callee(cxxMethodDecl(hasAnyName(names...))));
                };

                if (utils::findFirst(onPath("applyTransform", "scaleToFit"), body, context) != nullptr)
                    return nullptr;

                for (const auto& nodes : match(findAll(onPath("addRectangle", "addRoundedRectangle", "addEllipse").bind("add")), body, context))
                {
                    const auto* add = nodes.getNodeAs<CXXMemberCallExpr>("add");

                    if (add->getNumArgs() > 0 && isUnreducedBounds(*add->getArg(0), analyzer))
                        return add;
                }

                return nullptr;
            }
        };
    } // namespace clipped_stroke

    using clipped_stroke::ClippedStrokeCheck;
} // namespace clang::tidy::juce
