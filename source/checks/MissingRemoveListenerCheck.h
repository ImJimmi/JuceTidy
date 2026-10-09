#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace missing_remove_listener
    {
        class MissingRemoveListenerCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(matchesName("::add[A-Za-z]*(Listener|Callback|Client)$"))
                                                 .bind("method")),
                                      hasAnyArgument(ignoringParenImpCasts(cxxThisExpr())),
                                      forCallable(cxxMethodDecl(ofClass(cxxRecordDecl(unless(isLambda())).bind("owner")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");
                auto& context = *result.Context;

                if (classify(*call->getImplicitObjectArgument(), context) != Broadcaster::external)
                    return;

                const auto removeName = "remove" + method->getName().drop_front(3).str();
                const auto search = utils::searchDestructor(*owner,
                                                            callExpr(callee(functionDecl(hasName(removeName)))),
                                                            context);

                if (search != utils::DestructorSearch::notFound)
                    return;

                diag(call->getExprLoc(),
                     "call to %0 has no matching call to '%1' so callbacks "
                     "could be called on a destroyed object")
                    << method
                    << removeName;
            }

        private:
            enum class Broadcaster
            {
                owned,
                external,
                unknown,
            };

            [[nodiscard]] static bool isOwningContainer(QualType type)
            {
                static const std::vector<std::string> owningTypes{
                    "std::unique_ptr",
                    "std::array",
                    "std::vector",
                    "std::deque",
                    "juce::OwnedArray",
                    "juce::Array",
                };
                return utils::isSameOrDerivedFrom(type.getNonReferenceType()->getAsCXXRecordDecl(), owningTypes);
            }

            [[nodiscard]] static bool ownsPointees(QualType type)
            {
                static const std::vector<std::string> owningTypes{
                    "std::unique_ptr",
                    "juce::OwnedArray",
                };
                return utils::isSameOrDerivedFrom(type.getNonReferenceType()->getAsCXXRecordDecl(), owningTypes);
            }

            [[nodiscard]] static Broadcaster classifyElements(const Expr& container,
                                                              QualType elementType,
                                                              ASTContext& context)
            {
                const auto classification = classify(container, context);

                if (classification == Broadcaster::owned
                    && elementType->isPointerType()
                    && !ownsPointees(container.getType()))
                {
                    return Broadcaster::unknown;
                }

                return classification;
            }

            [[nodiscard]] static Broadcaster classifyLoopVariable(const VarDecl& variable, ASTContext& context)
            {
                for (const auto& declaration : context.getParents(variable))
                {
                    for (const auto& parent : context.getParents(declaration))
                    {
                        if (const auto* loop = parent.get<CXXForRangeStmt>();
                            loop != nullptr && loop->getLoopVariable() == &variable)
                        {
                            const auto* range = loop->getRangeInit()->IgnoreImplicit();

                            if (const auto* list = dyn_cast<CXXStdInitializerListExpr>(range))
                                range = list->getSubExpr()->IgnoreImplicit();

                            const auto* elements = dyn_cast<InitListExpr>(range);

                            if (elements == nullptr)
                                return classifyElements(*range, variable.getType().getNonReferenceType(), context);

                            const auto isOwned = llvm::all_of(elements->inits(),
                                                              [&context](const Expr* element) {
                                                                  const auto* stripped = element->IgnoreParenImpCasts();

                                                                  if (const auto* address = dyn_cast<UnaryOperator>(stripped);
                                                                      address != nullptr && address->getOpcode() == UO_AddrOf)
                                                                  {
                                                                      stripped = address->getSubExpr();
                                                                  }

                                                                  return classify(*stripped, context) == Broadcaster::owned;
                                                              });

                            return isOwned ? Broadcaster::owned : Broadcaster::unknown;
                        }
                    }
                }

                return Broadcaster::unknown;
            }

            [[nodiscard]] static Broadcaster classify(const Expr& object, ASTContext& context)
            {
                const auto* expression = object.IgnoreParenImpCasts();

                if (isa<CXXThisExpr>(expression))
                    return Broadcaster::owned;

                if (const auto* member = dyn_cast<MemberExpr>(expression))
                {
                    const auto* field = dyn_cast<FieldDecl>(member->getMemberDecl());

                    if (field == nullptr)
                        return Broadcaster::unknown;

                    if (field->getType()->isReferenceType() || field->getType()->isPointerType())
                        return Broadcaster::external;

                    return classify(*member->getBase(), context);
                }

                if (const auto* call = dyn_cast<CXXOperatorCallExpr>(expression))
                {
                    const auto isAccess = call->getOperator() == OO_Arrow
                                       || call->getOperator() == OO_Star
                                       || call->getOperator() == OO_Subscript;

                    if (!isAccess || !isOwningContainer(call->getArg(0)->getType()))
                        return Broadcaster::unknown;

                    return classifyElements(*call->getArg(0), call->getType(), context);
                }

                if (const auto* call = dyn_cast<CXXMemberCallExpr>(expression))
                {
                    return classify(*call->getImplicitObjectArgument(), context) == Broadcaster::external
                             ? Broadcaster::external
                             : Broadcaster::unknown;
                }

                if (const auto* call = dyn_cast<CallExpr>(expression))
                {
                    const auto* method = dyn_cast_or_null<CXXMethodDecl>(call->getDirectCallee());

                    return method == nullptr || method->isStatic()
                             ? Broadcaster::external
                             : Broadcaster::unknown;
                }

                if (const auto* reference = dyn_cast<DeclRefExpr>(expression))
                {
                    if (isa<ParmVarDecl>(reference->getDecl()))
                        return Broadcaster::external;

                    if (const auto* variable = dyn_cast<VarDecl>(reference->getDecl()))
                    {
                        return variable->hasGlobalStorage()
                                 ? Broadcaster::external
                                 : classifyLoopVariable(*variable, context);
                    }
                }

                return Broadcaster::unknown;
            }
        };
    } // namespace missing_remove_listener

    using missing_remove_listener::MissingRemoveListenerCheck;
} // namespace clang::tidy::juce
