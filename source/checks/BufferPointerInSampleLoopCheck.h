#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/StmtCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/SmallPtrSet.h>

namespace clang::tidy::juce
{
    namespace buffer_pointer_in_sample_loop
    {
        class BufferPointerInSampleLoopCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("getWritePointer",
                                                                      "getReadPointer",
                                                                      "getSample",
                                                                      "setSample",
                                                                      "addSample"),
                                                           ofClass(hasName("::juce::AudioBuffer")))
                                                 .bind("method")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                auto& context = *result.Context;
                const auto* loop = findInnermostLoop(*call, context);

                if (loop == nullptr
                    || utils::isInMacroBody(call->getExprLoc(),
                                            *result.SourceManager))
                {
                    return;
                }

                const auto variables = getLoopVariables(*loop, context);

                if (method->getName().ends_with("Pointer"))
                {
                    if (llvm::any_of(call->arguments(),
                                     [&](const Expr* argument) {
                                         return referencesAny(*argument, variables, context);
                                     }))
                    {
                        return;
                    }

                    diag(call->getExprLoc(),
                         "%0 is called on every iteration of the loop with the "
                         "same arguments. Get the pointer once before the loop")
                        << method;

                    return;
                }

                if (call->getNumArgs() < 2
                    || !referencesAny(*call->getArg(1),
                                      variables,
                                      context))
                {
                    return;
                }

                diag(call->getExprLoc(),
                     "calling %0 for every sample is slower than using the "
                     "pointer from %1")
                    << method
                    << (method->getNameAsString() == "getSample"
                            ? "'getReadPointer'"
                            : "'getWritePointer'");
            }

        private:
            [[nodiscard]] static const Stmt* findInnermostLoop(const Stmt& statement,
                                                               ASTContext& context)
            {
                auto parents = context.getParents(statement);

                while (!parents.empty())
                {
                    const auto& parent = *parents.begin();

                    if (parent.get<FunctionDecl>() != nullptr
                        || parent.get<LambdaExpr>() != nullptr)
                    {
                        return nullptr;
                    }

                    if (const auto* loop = parent.get<Stmt>();
                        loop != nullptr
                        && isa<ForStmt, CXXForRangeStmt>(loop))
                    {
                        return loop;
                    }

                    parents = context.getParents(parent);
                }

                return nullptr;
            }

            [[nodiscard]] static llvm::SmallPtrSet<const Decl*, 4> getLoopVariables(const Stmt& loop,
                                                                                    ASTContext& context)
            {
                using namespace clang::ast_matchers;

                llvm::SmallPtrSet<const Decl*, 4> variables;

                if (const auto* rangeLoop = dyn_cast<CXXForRangeStmt>(&loop))
                {
                    variables.insert(rangeLoop->getLoopVariable());
                    return variables;
                }

                const auto& forLoop = cast<ForStmt>(loop);

                for (const auto* part : {
                         forLoop.getInit(),
                         static_cast<const Stmt*>(forLoop.getInc()),
                     })
                {
                    if (part == nullptr)
                        continue;

                    for (const auto& nodes : match(findAll(stmt(anyOf(declStmt(forEach(varDecl().bind("variable"))),
                                                                      declRefExpr(to(varDecl().bind("variable")))))),
                                                   *part,
                                                   context))
                    {
                        variables.insert(nodes.getNodeAs<VarDecl>("variable"));
                    }
                }

                return variables;
            }

            [[nodiscard]] static bool referencesAny(const Expr& expression,
                                                    const llvm::SmallPtrSetImpl<const Decl*>& variables,
                                                    ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = match(findAll(declRefExpr().bind("reference")),
                                              expression,
                                              context);

                return llvm::any_of(references,
                                    [&variables](const BoundNodes& nodes) {
                                        return variables.contains(nodes
                                                                      .getNodeAs<DeclRefExpr>("reference")
                                                                      ->getDecl());
                                    });
            }
        };

    } // namespace buffer_pointer_in_sample_loop

    using buffer_pointer_in_sample_loop::BufferPointerInSampleLoopCheck;
} // namespace clang::tidy::juce
