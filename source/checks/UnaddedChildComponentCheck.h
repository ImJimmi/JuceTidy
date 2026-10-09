#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unadded_child_component
    {
        class UnaddedChildComponentCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 isSameOrDerivedFrom(hasName("::juce::Component")))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                std::vector<const FieldDecl*> children;

                for (const auto* field : record->fields())
                {
                    if (utils::isComponentOrOwnedComponent(field->getType())
                        && !isWindow(*field, *result.Context)
                        && !field->getLocation().isMacroID())
                    {
                        children.push_back(field);
                    }
                }

                if (children.empty())
                    return;

                const auto statements = utils::getClassStatements(*record);

                if (!statements)
                    return;

                for (const auto* child : children)
                {
                    if (mayBeAdded(*child, *statements, *result.Context))
                        continue;

                    diag(child->getLocation(),
                         "%0 is never added as a child with "
                         "'addAndMakeVisible()' or 'addChildComponent()'")
                        << child;
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> positioningCall()
            {
                using namespace clang::ast_matchers;

                return cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("setBounds",
                                                                         "setBoundsRelative",
                                                                         "setTopLeftPosition",
                                                                         "setCentrePosition",
                                                                         "setSize",
                                                                         "setBoundsInset",
                                                                         "setBoundsToFit",
                                                                         "centreWithSize"))));
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> layoutItem()
            {
                using namespace clang::ast_matchers;

                return cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasAnyName("::juce::FlexItem",
                                                                                             "::juce::GridItem")))));
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Decl> attachment()
            {
                using namespace clang::ast_matchers;

                return cxxRecordDecl(hasAnyName("::juce::AudioProcessorValueTreeState::SliderAttachment",
                                                "::juce::AudioProcessorValueTreeState::ButtonAttachment",
                                                "::juce::AudioProcessorValueTreeState::ComboBoxAttachment",
                                                "::juce::SliderParameterAttachment",
                                                "::juce::ButtonParameterAttachment",
                                                "::juce::ComboBoxParameterAttachment"));
            }

            [[nodiscard]] static bool isWindow(const FieldDecl& field, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto window = hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(isSameOrDerivedFrom(hasAnyName("::juce::TopLevelWindow",
                                                                                                                                       "::juce::TooltipWindow"))))));
                const auto ownedWindow = hasUnqualifiedDesugaredType(recordType(hasDeclaration(classTemplateSpecializationDecl(hasName("::std::unique_ptr"),
                                                                                                                               hasTemplateArgument(0, refersToType(window))))));

                return !match(fieldDecl(hasType(qualType(anyOf(window, ownedWindow)))), field, context).empty();
            }

            [[nodiscard]] static bool mayBeAdded(const FieldDecl& field,
                                                 ArrayRef<const Stmt*> statements,
                                                 ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = [&field](const Expr* expression) {
                    return utils::getReferencedField(*expression) == &field;
                };

                const auto nonAddingCall = expr(anyOf(positioningCall(),
                                                      layoutItem(),
                                                      cxxOperatorCallExpr(),
                                                      cxxConstructExpr(hasType(attachment())),
                                                      callExpr(callee(functionDecl(hasName("::std::make_unique"),
                                                                                   hasTemplateArgument(0,
                                                                                                       refersToType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(attachment()))))))))));

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(expr(anyOf(callExpr(),
                                                                      cxxConstructExpr()),
                                                                unless(nonAddingCall))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(utils::getCallArguments(*nodes.getNodeAs<Expr>("call")), references))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(initListExpr().bind("list")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(nodes.getNodeAs<InitListExpr>("list")->inits(), references))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(cxxMemberCallExpr(callee(cxxMethodDecl(hasName("addToDesktop"))))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (references(nodes.getNodeAs<CXXMemberCallExpr>("call")->getImplicitObjectArgument()))
                            return true;
                    }
                }

                return false;
            }
        };
    } // namespace unadded_child_component

    using unadded_child_component::UnaddedChildComponentCheck;
} // namespace clang::tidy::juce
