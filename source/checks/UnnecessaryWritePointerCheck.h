#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>
#include <llvm/ADT/STLExtras.h>

namespace clang::tidy::juce
{
    namespace unnecessary_write_pointer
    {
        class UnnecessaryWritePointerCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("getWritePointer",
                                                                      "getArrayOfWritePointers"),
                                                           ofClass(hasName("::juce::AudioBuffer")))
                                                 .bind("method")),
                                      forCallable(functionDecl(hasBody(stmt().bind("body")))),
                                      hasAncestor(stmt(equalsBoundNode("body"))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                const auto& body = *result.Nodes.getNodeAs<Stmt>("body");
                auto& context = *result.Context;

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager)
                    || isDiscarded(*call, context))
                {
                    return;
                }

                ExprMutationAnalyzer analyzer{ body, context };
                const auto isArray = method->getName() == "getArrayOfWritePointers";

                if (isArray
                        ? areChannelsWritten(*call, body, analyzer, context)
                        : isChannelWritten(*call, body, analyzer, context))
                {
                    return;
                }

                diag(call->getExprLoc(),
                     "the pointer acquired by '%0()' is never written to. Use "
                     "'%1()' instead, as '%0()' removes the buffer's isClear "
                     "flag")
                    << method->getName()
                    << (isArray ? "getArrayOfReadPointers" : "getReadPointer");
            }

        private:
            [[nodiscard]] static const Expr& getOutermostImplicitExpr(const Expr& expression, ASTContext& context)
            {
                const auto parent = utils::getSingleParent(expression, context);
                const auto* outer = parent ? parent->get<Expr>() : nullptr;

                if (isa_and_nonnull<ParenExpr, ImplicitCastExpr, MaterializeTemporaryExpr>(outer))
                    return getOutermostImplicitExpr(*outer, context);

                return expression;
            }

            [[nodiscard]] static std::optional<DynTypedNode> getUser(const Expr& expression, ASTContext& context)
            {
                return utils::getSingleParent(getOutermostImplicitExpr(expression, context), context);
            }

            [[nodiscard]] static bool isDiscarded(const Expr& expression, ASTContext& context)
            {
                const auto user = getUser(expression, context);

                return !user
                    || (user->get<Expr>() == nullptr
                        && user->get<VarDecl>() == nullptr
                        && user->get<ReturnStmt>() == nullptr);
            }

            [[nodiscard]] static bool isSpan(QualType type)
            {
                const auto* record = type->getAsCXXRecordDecl();
                return record != nullptr && record->isInStdNamespace() && record->getName() == "span";
            }

            [[nodiscard]] static bool hasReadOnlyElements(QualType span)
            {
                const auto* specialisation = dyn_cast_or_null<ClassTemplateSpecializationDecl>(span->getAsCXXRecordDecl());
                return specialisation != nullptr && specialisation->getTemplateArgs()[0].getAsType().isConstQualified();
            }

            [[nodiscard]] static const VarDecl* getInitialisedVariable(const Expr& expression, ASTContext& context)
            {
                const auto user = getUser(expression, context);
                const auto* variable = user ? user->get<VarDecl>() : nullptr;

                return variable != nullptr
                            && variable->hasLocalStorage()
                            && (variable->getType()->isPointerType() || isSpan(variable->getType()))
                         ? variable
                         : nullptr;
            }

            [[nodiscard]] static std::vector<const DeclRefExpr*> getReferences(const VarDecl& variable, const Stmt& body, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto self = varDecl(equalsNode(&variable));
                std::vector<const DeclRefExpr*> references;

                for (const auto& nodes : match(findAll(declRefExpr(to(self), unless(hasAncestor(self))).bind("reference")), body, context))
                    references.push_back(nodes.getNodeAs<DeclRefExpr>("reference"));

                return references;
            }

            [[nodiscard]] static const CXXForRangeStmt* getRangeLoop(const Expr& range, const Stmt& body, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                return dyn_cast_or_null<CXXForRangeStmt>(utils::findFirst(cxxForRangeStmt(hasRangeInit(equalsNode(&range))), body, context));
            }

            [[nodiscard]] static bool isChannelWritten(const Expr& channel,
                                                       const Stmt& body,
                                                       ExprMutationAnalyzer& analyzer,
                                                       ASTContext& context)
            {
                if (const auto* variable = getInitialisedVariable(channel, context))
                {
                    return llvm::any_of(getReferences(*variable, body, context),
                                        [&](const DeclRefExpr* reference) {
                                            return isChannelWritten(*reference, body, analyzer, context);
                                        });
                }

                const auto& outer = getOutermostImplicitExpr(channel, context);
                const auto user = utils::getSingleParent(outer, context);

                if (const auto* span = user ? user->get<CXXConstructExpr>() : nullptr;
                    span != nullptr && isSpan(span->getType()) && span->getArg(0) == &outer)
                {
                    return isSpanWritten(*span, body, analyzer, context);
                }

                if (const auto* address = user ? user->get<UnaryOperator>() : nullptr;
                    address != nullptr && address->getOpcode() == UO_AddrOf)
                {
                    return areChannelsWritten(*address, body, analyzer, context);
                }

                return analyzer.isPointeeMutated(&channel);
            }

            [[nodiscard]] static bool isElementWritten(const VarDecl& element, ExprMutationAnalyzer& analyzer)
            {
                const auto type = element.getType();

                return type->isReferenceType()
                    && !type.getNonReferenceType().isConstQualified()
                    && analyzer.isMutated(&element);
            }

            [[nodiscard]] static bool isSpanAccessWritten(const CallExpr& access,
                                                          const Stmt& body,
                                                          ExprMutationAnalyzer& analyzer,
                                                          ASTContext& context)
            {
                const auto* function = access.getDirectCallee();
                const auto name = function != nullptr && function->getDeclName().isIdentifier()
                                    ? function->getName()
                                    : StringRef{};

                if (llvm::is_contained({ "size", "ssize", "size_bytes", "empty" }, name))
                    return false;

                if (llvm::is_contained({ "front", "back" }, name))
                    return analyzer.isMutated(&access);

                if (name == "data")
                    return isChannelWritten(access, body, analyzer, context);

                if (llvm::is_contained({ "first", "last", "subspan" }, name))
                    return isSpanWritten(access, body, analyzer, context);

                return true;
            }

            [[nodiscard]] static bool isSpanWritten(const Expr& span,
                                                    const Stmt& body,
                                                    ExprMutationAnalyzer& analyzer,
                                                    ASTContext& context)
            {
                if (hasReadOnlyElements(span.getType()))
                    return false;

                if (const auto* variable = getInitialisedVariable(span, context))
                {
                    return llvm::any_of(getReferences(*variable, body, context),
                                        [&](const DeclRefExpr* reference) {
                                            return isSpanWritten(*reference, body, analyzer, context);
                                        });
                }

                const auto& outer = getOutermostImplicitExpr(span, context);
                const auto user = utils::getSingleParent(outer, context);

                if (!user)
                    return true;

                if (const auto* copy = user->get<CXXConstructExpr>();
                    copy != nullptr && isSpan(copy->getType()) && copy->getNumArgs() == 1)
                {
                    return isSpanWritten(*copy, body, analyzer, context);
                }

                if (const auto* subscript = user->get<CXXOperatorCallExpr>();
                    subscript != nullptr && subscript->getOperator() == OO_Subscript && subscript->getArg(0) == &outer)
                {
                    return analyzer.isMutated(subscript);
                }

                if (const auto* member = user->get<MemberExpr>())
                {
                    const auto parent = utils::getSingleParent(*member, context);
                    const auto* call = parent ? parent->get<CXXMemberCallExpr>() : nullptr;

                    return call == nullptr || isSpanAccessWritten(*call, body, analyzer, context);
                }

                if (const auto* call = user->get<CallExpr>();
                    call != nullptr && call->getNumArgs() == 1 && call->getDirectCallee() != nullptr && call->getDirectCallee()->isInStdNamespace())
                {
                    return isSpanAccessWritten(*call, body, analyzer, context);
                }

                if (const auto* loop = getRangeLoop(outer, body, context))
                    return isElementWritten(*loop->getLoopVariable(), analyzer);

                return true;
            }

            [[nodiscard]] static bool hasReadOnlySamples(const Expr& channels)
            {
                const auto channel = channels.getType()->getPointeeType();
                return !channel.isNull() && channel->getPointeeType().isConstQualified();
            }

            [[nodiscard]] static bool areChannelsWritten(const Expr& channels,
                                                         const Stmt& body,
                                                         ExprMutationAnalyzer& analyzer,
                                                         ASTContext& context)
            {
                if (const auto* variable = getInitialisedVariable(channels, context))
                {
                    return llvm::any_of(getReferences(*variable, body, context),
                                        [&](const DeclRefExpr* reference) {
                                            return areChannelsWritten(*reference, body, analyzer, context);
                                        });
                }

                const auto& outer = getOutermostImplicitExpr(channels, context);

                if (hasReadOnlySamples(outer))
                    return false;

                const auto user = utils::getSingleParent(outer, context);

                if (const auto* subscript = user ? user->get<ArraySubscriptExpr>() : nullptr;
                    subscript != nullptr && subscript->getBase() == &outer)
                {
                    return isChannelWritten(*subscript, body, analyzer, context);
                }

                if (const auto* dereference = user ? user->get<UnaryOperator>() : nullptr;
                    dereference != nullptr && dereference->getOpcode() == UO_Deref)
                {
                    return isChannelWritten(*dereference, body, analyzer, context);
                }

                return true;
            }
        };
    } // namespace unnecessary_write_pointer

    using unnecessary_write_pointer::UnnecessaryWritePointerCheck;
} // namespace clang::tidy::juce
