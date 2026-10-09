#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace paint_side_effects
    {
        class PaintSideEffectsCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const std::vector<std::string> paintMethods{
                    "juce::Component::paint",
                    "juce::Component::paintOverChildren",
                };

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasAnyName("repaint",
                                                                      "setBounds",
                                                                      "addAndMakeVisible",
                                                                      "setVisible"),
                                                           ofClass(isSameOrDerivedFrom(hasName("::juce::Component"))))
                                                 .bind("method")),
                                      forCallable(cxxMethodDecl(utils::isOrOverridesAnyOf(paintMethods)).bind("paint")))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>("method");
                const auto* paint = result.Nodes.getNodeAs<CXXMethodDecl>("paint");

                if (utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                    return;

                if (method->getName() == "repaint")
                {
                    diag(call->getExprLoc(),
                         "calling 'repaint()' inside '%0()' causes an endless "
                         "loop. Call 'repaint()' from a 'juce::Timer' callback "
                         "if you need to repaint repeatedly")
                        << paint->getNameAsString();
                }
                else if (method->getName() == "setBounds")
                {
                    diag(call->getExprLoc(),
                         "calling 'setBounds()' inside '%0()' changes the "
                         "layout while painting, triggering another repaint. "
                         "Do this in 'resized()' instead")
                        << paint->getNameAsString();
                }
                else if (method->getName() == "addAndMakeVisible")
                {
                    diag(call->getExprLoc(),
                         "calling 'addAndMakeVisible()' inside '%0()' changes "
                         "the component hierarchy while painting, triggering "
                         "another repaint. Do this elsewhere")
                        << paint->getNameAsString();
                }
                else if (method->getName() == "setVisible")
                {
                    diag(call->getExprLoc(),
                         "calling 'setVisible()' inside '%0()' triggers "
                         "another repaint. Do this elsewhere")
                        << paint->getNameAsString();
                }
            }
        };
    } // namespace paint_side_effects

    using paint_side_effects::PaintSideEffectsCheck;
} // namespace clang::tidy::juce
