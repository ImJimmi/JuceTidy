#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/Attr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace ignore_unused
    {
        class IgnoreUnusedCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(hasName("::juce::ignoreUnused"))),
                             forCallable(functionDecl().bind("function")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CallExpr>("call");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                auto& context = *result.Context;

                if (utils::isInMacroBody(call->getBeginLoc(), *result.SourceManager))
                    return;

                const auto diagnostic = diag(call->getBeginLoc(),
                                             "use '[[maybe_unused]]' instead "
                                             "of 'juce::ignoreUnused()'");

                std::vector<const VarDecl*> variables;

                for (const auto* argument : call->arguments())
                {
                    const auto* variable = getAnnotatableVariable(*argument, *function, context);

                    if (variable == nullptr)
                        return;

                    variables.push_back(variable);
                }

                const auto statementRange = getStatementLineRange(*call, context);

                if (variables.empty() || !statementRange)
                    return;

                for (const auto* variable : variables)
                    diagnostic << FixItHint::CreateInsertion(variable->getBeginLoc(), "[[maybe_unused]] ");

                diagnostic << FixItHint::CreateRemoval(*statementRange);
            }

        private:
            [[nodiscard]] static const VarDecl* getAnnotatableVariable(const Expr& argument,
                                                                       const FunctionDecl& function,
                                                                       ASTContext& context)
            {
                const auto* reference = dyn_cast<DeclRefExpr>(argument.IgnoreParenImpCasts());
                const auto* variable = reference != nullptr
                                         ? dyn_cast<VarDecl>(reference->getDecl())
                                         : nullptr;

                if (variable == nullptr
                    || variable->hasAttr<UnusedAttr>()
                    || variable->getBeginLoc().isMacroID()
                    || variable->getDeclContext() != &function)
                {
                    return nullptr;
                }

                if (isa<ParmVarDecl>(variable))
                    return variable;

                const auto parent = utils::getSingleParent(*variable, context);
                const auto* declaration = parent ? parent->get<DeclStmt>() : nullptr;

                return declaration != nullptr && declaration->isSingleDecl()
                         ? variable
                         : nullptr;
            }

            [[nodiscard]] static std::optional<CharSourceRange> getStatementLineRange(const CallExpr& call,
                                                                                      ASTContext& context)
            {
                const auto& sourceManager = context.getSourceManager();
                const auto parent = utils::getSingleParent(call, context);

                if (!parent
                    || parent->get<CompoundStmt>() == nullptr
                    || call.getBeginLoc().isMacroID()
                    || !utils::isFirstOnLine(call.getBeginLoc(), sourceManager))
                {
                    return std::nullopt;
                }

                const auto semicolon = Lexer::findNextToken(call.getEndLoc(), sourceManager, context.getLangOpts());

                if (!semicolon || semicolon->isNot(tok::semi))
                    return std::nullopt;

                const auto lineStart = call.getBeginLoc().getLocWithOffset(-static_cast<int>(utils::getLineIndentation(call.getBeginLoc(), sourceManager).size()));
                const auto afterSemicolon = semicolon->getEndLoc();
                const auto* end = sourceManager.getCharacterData(afterSemicolon);
                auto length = 0;

                while (end[length] == ' ' || end[length] == '\t')
                    ++length;

                if (end[length] == '\n')
                    return CharSourceRange::getCharRange(lineStart, afterSemicolon.getLocWithOffset(length + 1));

                return std::nullopt;
            }
        };
    } // namespace ignore_unused

    using ignore_unused::IgnoreUnusedCheck;
} // namespace clang::tidy::juce
