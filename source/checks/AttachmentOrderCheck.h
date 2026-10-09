#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace attachment_order
    {
        class AttachmentOrderCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto attachment = cxxRecordDecl(hasAnyName("::juce::AudioProcessorValueTreeState::SliderAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ButtonAttachment",
                                                                 "::juce::AudioProcessorValueTreeState::ComboBoxAttachment",
                                                                 "::juce::SliderParameterAttachment",
                                                                 "::juce::ButtonParameterAttachment",
                                                                 "::juce::ComboBoxParameterAttachment"));

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(cxxConstructExpr(hasType(attachment)),
                               callExpr(callee(functionDecl(hasName("::std::make_unique"),
                                                            hasTemplateArgument(0,
                                                                                refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(attachment))))))))))
                        .bind("construction"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* construction = result.Nodes.getNodeAs<Expr>("construction");
                const auto* component = findAttachedComponent(*construction);
                const auto* attachment = component != nullptr
                                           ? findStoringField(*construction, *result.Context)
                                           : nullptr;

                if (attachment == nullptr
                    || attachment->getParent() != component->getParent()
                    || attachment->getFieldIndex() > component->getFieldIndex())
                {
                    return;
                }

                diag(attachment->getLocation(),
                     "%0 is declared before %1 which it's attached to. Declare "
                     "%0 after %1")
                    << attachment
                    << component;

                diag(construction->getBeginLoc(),
                     "%0 is attached to %1 here",
                     DiagnosticIDs::Note)
                    << attachment << component;
            }

        private:
            [[nodiscard]] static const FieldDecl* findAttachedComponent(const Expr& construction)
            {
                const auto arguments = utils::getCallArguments(construction);

                for (const auto* argument : arguments)
                {
                    if (const auto* field = utils::getReferencedField(*argument);
                        field != nullptr && utils::isComponentOrOwnedComponent(field->getType()))
                    {
                        return field;
                    }
                }

                return nullptr;
            }

            [[nodiscard]] static const FieldDecl* findStoringField(const Expr& construction, ASTContext& context)
            {
                DynTypedNode node = DynTypedNode::create(construction);

                while (true)
                {
                    const auto parents = context.getParents(node);

                    if (const auto* field = llvm::find_if(parents,
                                                          [](const DynTypedNode& candidate) {
                                                              return candidate.get<FieldDecl>() != nullptr;
                                                          });
                        field != parents.end())
                    {
                        return field->get<FieldDecl>();
                    }

                    if (parents.empty())
                        return nullptr;

                    const auto& parent = *parents.begin();

                    if (const auto* constructor = parent.get<CXXConstructorDecl>())
                    {
                        const auto* initialiser = llvm::find_if(constructor->inits(),
                                                                [&node](const CXXCtorInitializer* candidate) {
                                                                    return candidate->getInit() == node.get<Expr>();
                                                                });

                        return initialiser != constructor->init_end()
                                 ? (*initialiser)->getMember()
                                 : nullptr;
                    }

                    if (const auto* field = parent.get<FieldDecl>())
                        return field;

                    if (const auto* assignment = parent.get<BinaryOperator>();
                        assignment != nullptr && assignment->isAssignmentOp())
                    {
                        return utils::getReferencedField(*assignment->getLHS());
                    }

                    if (const auto* assignment = parent.get<CXXOperatorCallExpr>();
                        assignment != nullptr && assignment->getOperator() == OO_Equal)
                    {
                        return utils::getReferencedField(*assignment->getArg(0));
                    }

                    if (const auto* call = parent.get<CXXMemberCallExpr>();
                        call != nullptr
                        && call->getMethodDecl() != nullptr
                        && call->getMethodDecl()->getName() == "reset")
                    {
                        return utils::getReferencedField(*call->getImplicitObjectArgument());
                    }

                    const auto* expression = parent.get<Expr>();

                    if (expression == nullptr
                        || !isa<ImplicitCastExpr,
                                MaterializeTemporaryExpr,
                                CXXBindTemporaryExpr,
                                ExprWithCleanups,
                                CXXNewExpr,
                                CXXConstructExpr,
                                CXXFunctionalCastExpr,
                                ParenExpr>(expression))
                    {
                        return nullptr;
                    }

                    node = parent;
                }
            }
        };
    } // namespace attachment_order

    using attachment_order::AttachmentOrderCheck;
} // namespace clang::tidy::juce
