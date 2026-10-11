#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace processor_editor_coupling
    {
        class ProcessorEditorCouplingCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto processor = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessor")));
                const auto editor = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::AudioProcessorEditor")));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("::juce::AudioProcessor::getActiveEditor"))),
                                      hasAncestor(cxxMethodDecl(ofClass(processor))))
                        .bind("editorAccess"),
                    this);

                finder->addMatcher(
                    binaryOperation(unless(isInTemplateInstantiation()),
                                    isAssignmentOperator(),
                                    hasLHS(memberExpr(hasObjectExpression(anyOf(hasType(processor),
                                                                                hasType(pointsTo(processor)))),
                                                      member(fieldDecl().bind("field")))),
                                    hasRHS(ignoringImplicit(anyOf(lambdaExpr(hasAnyCapture(capturesThis())),
                                                                  cxxThisExpr()))),
                                    hasAncestor(cxxMethodDecl(ofClass(editor.bind("editor")))))
                        .bind("assignment"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto& sourceManager = *result.SourceManager;

                if (const auto* access = result.Nodes.getNodeAs<CXXMemberCallExpr>("editorAccess"))
                {
                    if (utils::isInMacroBody(access->getExprLoc(), sourceManager))
                        return;

                    diag(access->getExprLoc(),
                         "plugin processors should avoid calling into their "
                         "editors directly. Instead, use "
                         "juce::ChangeBroadcaster or some other listener "
                         "mechanism to have the processor broadcast changes, "
                         "or have the editor read from its processor");

                    return;
                }

                const auto* assignment = result.Nodes.getNodeAs<Expr>("assignment");
                const auto* field = result.Nodes.getNodeAs<FieldDecl>("field");
                const auto* editor = result.Nodes.getNodeAs<CXXRecordDecl>("editor");

                if (utils::isInMacroBody(assignment->getExprLoc(), sourceManager))
                    return;

                const auto reset = binaryOperation(isAssignmentOperator(),
                                                   hasLHS(memberExpr(member(equalsNode(field)))));

                if (utils::searchDestructor(*editor, reset, *result.Context) != utils::DestructorSearch::notFound)
                    return;

                diag(assignment->getBeginLoc(),
                     "%0 still refers to the editor after the editor is "
                     "destroyed. Reset it in the editor's destructor")
                    << field;
            }
        };
    } // namespace processor_editor_coupling

    using processor_editor_coupling::ProcessorEditorCouplingCheck;
} // namespace clang::tidy::juce
