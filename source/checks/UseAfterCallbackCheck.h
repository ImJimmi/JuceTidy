#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/DenseMap.h>

namespace clang::tidy::juce
{
    namespace use_after_callback
    {
        class UseAfterCallbackCheck : public JuceTidyCheck
        {
        public:
            UseAfterCallbackCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawGuardTypes{ Options.get("GuardTypes", defaultGuardTypes) }
                , guardTypes{ utils::parseQualifiedNames(rawGuardTypes) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "GuardTypes", rawGuardTypes);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMethodDecl(isDefinition(),
                                  unless(isInstantiated()),
                                  unless(isExpansionInSystemHeader()),
                                  unless(cxxDestructorDecl()),
                                  hasBody(stmt()))
                        .bind("method"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                auto& context = *result.Context;

                if (hasGuard(*method, context))
                    return;

                for (const auto* invocation : findInvocations(*method, context))
                {
                    const auto* access = findLaterMemberAccess(*invocation, *method, context);

                    if (access == nullptr || utils::isInMacroBody(access->getExprLoc(), context.getSourceManager()))
                        continue;

                    diag(access->getExprLoc(),
                         "a callback called earlier in this function may have "
                         "deleted 'this', but %0 is used afterwards. Check that "
                         "'this' still exists first, for example with a "
                         "juce::WeakReference or juce::Component::BailOutChecker")
                        << access->getMemberDecl();

                    diag(invocation->getExprLoc(),
                         "callback called here",
                         DiagnosticIDs::Note);
                }
            }

            void onStartOfTranslationUnit() override
            {
                invokesCallbackCache.clear();
            }

        private:
            static inline constexpr auto defaultGuardTypes =
                "::juce::WeakReference;"
                "::juce::Component::SafePointer;"
                "::juce::Component::BailOutChecker;"
                "::std::weak_ptr";

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> directCallback()
            {
                using namespace clang::ast_matchers;

                const auto function = hasType(hasCanonicalType(hasDeclaration(classTemplateSpecializationDecl(hasName("::std::function")))));

                return expr(anyOf(cxxOperatorCallExpr(hasOverloadedOperatorName("()"),
                                                      hasType(voidType()),
                                                      hasArgument(0, expr(function).bind("function"))),
                                  cxxMemberCallExpr(callee(cxxMethodDecl(hasName("call"),
                                                                         ofClass(hasName("::juce::ListenerList"))))),
                                  cxxMemberCallExpr(callee(cxxMethodDecl(hasName("sendSynchronousChangeMessage"),
                                                                         ofClass(hasName("::juce::ChangeBroadcaster")))))));
            }

            [[nodiscard]] std::vector<const Expr*> findInvocations(const CXXMethodDecl& method, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                std::vector<const Expr*> invocations;
                const auto inMethod = forCallable(equalsNode(&method));

                for (const auto& nodes : match(findAll(expr(directCallback(), inMethod).bind("invocation")), *method.getBody(), context))
                {
                    if (const auto* function = nodes.getNodeAs<Expr>("function"); function != nullptr && isLocalLambda(*function))
                        continue;

                    invocations.push_back(nodes.getNodeAs<Expr>("invocation"));
                }

                for (const auto& nodes : match(findAll(cxxMemberCallExpr(on(ignoringParenImpCasts(cxxThisExpr())), inMethod).bind("call")), *method.getBody(), context))
                {
                    const auto* call = nodes.getNodeAs<CXXMemberCallExpr>("call");

                    if (const auto* callee = call->getMethodDecl(); callee != nullptr && invokesCallback(*callee, context))
                        invocations.push_back(call);
                }

                return invocations;
            }

            [[nodiscard]] bool invokesCallback(const CXXMethodDecl& method, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const FunctionDecl* definition = nullptr;

                if (method.isVirtual()
                    || utils::isInJuceNamespace(method)
                    || !method.hasBody(definition)
                    || definition == nullptr)
                {
                    return false;
                }

                const auto* canonical = definition->getCanonicalDecl();

                if (const auto cached = invokesCallbackCache.find(canonical); cached != invokesCallbackCache.end())
                    return cached->second;

                invokesCallbackCache.insert({ canonical, false });
                const auto result = !findInvocations(*cast<CXXMethodDecl>(definition), context).empty();
                invokesCallbackCache.insert_or_assign(canonical, result);
                return result;
            }

            [[nodiscard]] static bool isLocalLambda(const Expr& function)
            {
                const auto* reference = dyn_cast<DeclRefExpr>(function.IgnoreParenImpCasts());
                const auto* variable = reference != nullptr ? dyn_cast<VarDecl>(reference->getDecl()) : nullptr;

                if (variable == nullptr || isa<ParmVarDecl>(variable) || !variable->isLocalVarDecl() || variable->getInit() == nullptr)
                    return false;

                const auto* init = variable->getInit()->IgnoreImplicit();

                while (const auto* construction = dyn_cast<CXXConstructExpr>(init))
                {
                    if (construction->getNumArgs() != 1)
                        return false;

                    init = construction->getArg(0)->IgnoreImplicit();
                }

                return isa<LambdaExpr>(init);
            }

            [[nodiscard]] bool hasGuard(const CXXMethodDecl& method, ASTContext& context) const
            {
                using namespace clang::ast_matchers;

                const auto guard = varDecl(hasType(hasCanonicalType(hasDeclaration(cxxRecordDecl(utils::isSameOrDerivedFromAnyOf(guardTypes))))));

                return utils::findFirst(declStmt(has(guard)), *method.getBody(), context) != nullptr;
            }

            [[nodiscard]] static const MemberExpr* findLaterMemberAccess(const Expr& invocation, const CXXMethodDecl& method, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto access = memberExpr(hasObjectExpression(ignoringParenImpCasts(cxxThisExpr())),
                                               forCallable(equalsNode(&method)))
                                        .bind("access");
                const Stmt* child = &invocation;
                auto parents = context.getParents(invocation);

                while (parents.size() == 1)
                {
                    const auto& parent = *parents.begin();

                    if (const auto* variable = parent.get<VarDecl>(); variable != nullptr && variable->isLocalVarDecl())
                    {
                        parents = context.getParents(*variable);
                        continue;
                    }

                    if (parent.get<Decl>() != nullptr || parent.get<LambdaExpr>() != nullptr)
                        return nullptr;

                    if (const auto* compound = parent.get<CompoundStmt>())
                    {
                        const auto* position = llvm::find(compound->body(), child);

                        for (const auto* sibling : llvm::make_range(std::next(position), compound->body_end()))
                            if (const auto* found = selectFirst<MemberExpr>("access", match(findAll(access), *sibling, context)))
                                return found;
                    }

                    child = parent.get<Stmt>();

                    if (child == nullptr)
                        return nullptr;

                    parents = context.getParents(*child);
                }

                return nullptr;
            }

            const StringRef rawGuardTypes;
            const std::vector<std::string> guardTypes;

            llvm::DenseMap<const FunctionDecl*, bool> invokesCallbackCache;
        };
    } // namespace use_after_callback

    using use_after_callback::UseAfterCallbackCheck;
} // namespace clang::tidy::juce
