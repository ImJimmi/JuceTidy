#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace untitled_control
    {
        class UntitledControlCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isTemplateInstantiation()),
                                                 has(fieldDecl()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                std::vector<const FieldDecl*> controls;

                for (const auto* field : record->fields())
                {
                    if (const auto* control = getControl(field->getType());
                        control != nullptr
                        && !field->getLocation().isMacroID()
                        && !titlesItself(*control, *result.Context))
                    {
                        controls.push_back(field);
                    }
                }

                if (controls.empty())
                    return;

                auto statements = utils::getClassStatements(*record);

                if (!statements)
                    return;

                for (const auto* field : record->fields())
                {
                    if (const auto* initialiser = field->getInClassInitializer())
                        statements->push_back(initialiser);
                }

                for (const auto* control : controls)
                {
                    const auto isButton = holdsButton(control->getType());

                    if (mayBeTitled(*control, *statements, *result.Context)
                        || (isButton && isGivenButtonText(*control, *record, *statements, *result.Context)))
                    {
                        continue;
                    }

                    diag(control->getLocation(),
                         "%0 is never given a title so it won't be accessible "
                         "by a screen reader. Call 'setTitle()'"
                         "%select{| or 'setButtonText()'}1 "
                         "to give the widget a concise name")
                        << control
                        << isButton;
                }
            }

        private:
            [[nodiscard]] static bool isControl(const CXXRecordDecl* record)
            {
                static const std::vector<std::string> controls{
                    "juce::Button",
                    "juce::ComboBox",
                    "juce::Slider",
                    "juce::TextEditor",
                };

                return utils::isSameOrDerivedFrom(record, controls);
            }

            [[nodiscard]] static const CXXRecordDecl* getControl(QualType type)
            {
                const auto* record = type->getAsCXXRecordDecl();

                if (const auto* specialisation = dyn_cast_or_null<ClassTemplateSpecializationDecl>(record);
                    specialisation != nullptr && utils::qualifiedName(*specialisation) == "std::unique_ptr")
                {
                    record = specialisation->getTemplateArgs().get(0).getAsType()->getAsCXXRecordDecl();
                }

                return isControl(record) ? record : nullptr;
            }

            [[nodiscard]] static bool holdsButton(QualType type)
            {
                static const std::vector<std::string> button{ "juce::Button" };
                return utils::isSameOrDerivedFrom(getControl(type), button);
            }

            [[nodiscard]] static bool titlesItself(const CXXRecordDecl& control, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto* definition = control.getDefinition();

                if (definition == nullptr || utils::isInJuceNamespace(*definition))
                    return false;

                const auto statements = utils::getClassStatements(*definition);

                if (!statements)
                    return true;

                const auto setsTitle = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("setTitle",
                                                                                         "setButtonText"))),
                                                         on(cxxThisExpr()));

                if (llvm::any_of(*statements, [&](const Stmt* statement) {
                        return utils::findFirst(setsTitle, *statement, context) != nullptr;
                    }))
                {
                    return true;
                }

                for (const auto* constructor : definition->ctors())
                {
                    for (const auto* initialiser : constructor->inits())
                    {
                        if (initialiser->isBaseInitializer()
                            && initialiser->isWritten()
                            && constructsWithText(*initialiser->getInit(), context))
                        {
                            return true;
                        }
                    }
                }

                return llvm::any_of(definition->bases(), [&](const CXXBaseSpecifier& base) {
                    const auto* baseRecord = utils::getRecord(base.getType());
                    return isControl(baseRecord) && titlesItself(*baseRecord, context);
                });
            }

            [[nodiscard]] static bool isJuceOrStd(const Expr& call)
            {
                const NamedDecl* callee = nullptr;

                if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
                    callee = construction->getConstructor()->getParent();
                else
                    callee = cast<CallExpr>(call).getDirectCallee();

                if (callee == nullptr)
                    return false;

                const auto name = utils::qualifiedName(*callee);
                return llvm::StringRef{ name }.starts_with("juce::") || llvm::StringRef{ name }.starts_with("std::");
            }

            [[nodiscard]] static bool mayBeTitled(const FieldDecl& field,
                                                  ArrayRef<const Stmt*> statements,
                                                  ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = [&field](const Expr* expression) {
                    return expression != nullptr && utils::getReferencedField(*expression) == &field;
                };

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("setTitle",
                                                                                                             "setButtonText"))))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (references(nodes.getNodeAs<CXXMemberCallExpr>("call")->getImplicitObjectArgument()))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(expr(anyOf(callExpr(), cxxConstructExpr())).bind("call")),
                                                   *statement,
                                                   context))
                    {
                        const auto* call = nodes.getNodeAs<Expr>("call");

                        if (!isJuceOrStd(*call) && llvm::any_of(utils::getCallArguments(*call), references))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(initListExpr().bind("list")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(nodes.getNodeAs<InitListExpr>("list")->inits(), references))
                            return true;
                    }
                }

                return false;
            }

            [[nodiscard]] static bool isGivenButtonText(const FieldDecl& field,
                                                        const CXXRecordDecl& record,
                                                        ArrayRef<const Stmt*> statements,
                                                        ASTContext& context)
            {
                using namespace clang::ast_matchers;

                if (const auto* initialiser = field.getInClassInitializer();
                    initialiser != nullptr && constructsWithText(*initialiser, context))
                {
                    return true;
                }

                for (const auto* constructor : record.ctors())
                {
                    const auto* definition = dyn_cast_or_null<CXXConstructorDecl>(constructor->getDefinition());

                    if (definition == nullptr)
                        continue;

                    for (const auto* initialiser : definition->inits())
                    {
                        if (initialiser->getMember() == &field
                            && initialiser->isWritten()
                            && constructsWithText(*initialiser->getInit(), context))
                        {
                            return true;
                        }
                    }
                }

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(cxxOperatorCallExpr(hasOverloadedOperatorName("="),
                                                                               hasArgument(1, expr().bind("value")))
                                                               .bind("assignment")),
                                                   *statement,
                                                   context))
                    {
                        const auto* assignment = nodes.getNodeAs<CXXOperatorCallExpr>("assignment");

                        if (utils::getReferencedField(*assignment->getArg(0)) == &field
                            && constructsWithText(*nodes.getNodeAs<Expr>("value"), context))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            [[nodiscard]] static bool constructsWithText(const Expr& expression, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto buttonWithText = cxxConstructExpr(hasArgument(0, expr()),
                                                             hasDeclaration(cxxConstructorDecl(unless(isCopyConstructor()),
                                                                                               unless(isMoveConstructor()),
                                                                                               ofClass(isSameOrDerivedFrom("::juce::Button")))));
                const auto makeUniqueWithText = callExpr(callee(functionDecl(hasName("::std::make_unique"))),
                                                         hasArgument(0, expr()));

                return utils::findFirst(expr(anyOf(buttonWithText, makeUniqueWithText)), expression, context) != nullptr;
            }
        };
    } // namespace untitled_control

    using untitled_control::UntitledControlCheck;
} // namespace clang::tidy::juce
