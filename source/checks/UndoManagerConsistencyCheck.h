#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace undo_manager_consistency
    {
        class UndoManagerConsistencyCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isInstantiated()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                auto& context = *result.Context;
                const auto valueTreeEdit = cxxMemberCallExpr(callee(cxxMethodDecl(ofClass(hasAnyName("::juce::ValueTree",
                                                                                                     "::juce::CachedValue")))))
                                               .bind("call");

                std::vector<const CXXMemberCallExpr*> withoutUndo;
                std::vector<const CXXMemberCallExpr*> withUndo;

                for (const auto* method : record->methods())
                {
                    const FunctionDecl* definition = nullptr;

                    if (method->isImplicit()
                        || isa<CXXConstructorDecl, CXXDestructorDecl>(method)
                        || !method->hasBody(definition)
                        || definition == nullptr)
                    {
                        continue;
                    }

                    for (const auto* statement : utils::getFunctionStatements(*definition))
                    {
                        for (const auto& nodes : match(findAll(valueTreeEdit), *statement, context))
                        {
                            const auto* call = nodes.getNodeAs<CXXMemberCallExpr>("call");
                            const auto* undoManager = getUndoManagerArgument(*call);

                            if (undoManager == nullptr)
                                continue;

                            if (isNull(*undoManager, context))
                                withoutUndo.push_back(call);
                            else
                                withUndo.push_back(call);
                        }
                    }
                }

                if (withUndo.empty())
                    return;

                const auto undoText = utils::getSourceText(getUndoManagerArgument(*withUndo.front())->getSourceRange(),
                                                           *result.SourceManager,
                                                           context.getLangOpts())
                                          .value_or("an UndoManager");

                for (const auto* call : withoutUndo)
                {
                    if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                        continue;

                    diag(call->getExprLoc(),
                         "'%0()' is called without an UndoManager, unlike "
                         "other changes in %1 that use '%2'")
                        << call->getMethodDecl()->getName()
                        << record
                        << undoText;

                    for (const auto* undoableCall : withUndo)
                    {
                        diag(undoableCall->getExprLoc(),
                             "'%0()' is called with an UndoManager here",
                             DiagnosticIDs::Note)
                            << undoableCall->getMethodDecl()->getName();
                    }
                }
            }

        private:
            [[nodiscard]] static const Expr* getUndoManagerArgument(const CXXMemberCallExpr& call)
            {
                const auto* method = call.getMethodDecl();

                for (unsigned int index = 0; index < method->getNumParams() && index < call.getNumArgs(); ++index)
                {
                    const auto type = method->getParamDecl(index)->getType();

                    if (type->isPointerType()
                        && utils::isSameOrDerivedFrom(utils::getRecord(type->getPointeeType()), { "juce::UndoManager" }))
                    {
                        return call.getArg(index);
                    }
                }

                return nullptr;
            }

            [[nodiscard]] static bool isNull(const Expr& argument, ASTContext& context)
            {
                const auto* value = isa<CXXDefaultArgExpr>(argument)
                                      ? cast<CXXDefaultArgExpr>(argument).getExpr()
                                      : &argument;

                return value->IgnoreParenImpCasts()->isNullPointerConstant(context, Expr::NPC_ValueDependentIsNotNull)
                    != Expr::NPCK_NotNull;
            }
        };
    } // namespace undo_manager_consistency

    using undo_manager_consistency::UndoManagerConsistencyCheck;
} // namespace clang::tidy::juce
