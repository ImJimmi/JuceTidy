#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace expensive_paint
    {
        class ExpensivePaintCheck : public JuceTidyCheck
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
                const auto inPaint = forCallable(cxxMethodDecl(utils::isOrOverridesAnyOf(paintMethods)).bind("paint"));

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(hasAnyName("::juce::ImageFileFormat::loadFrom",
                                                            "::juce::Drawable::createFromImageData",
                                                            "::juce::Drawable::createFromImageFile",
                                                            "::juce::Drawable::createFromSVG",
                                                            "::juce::Drawable::createFromSVGFile",
                                                            "::juce::parseXML",
                                                            "::juce::XmlDocument::parse",
                                                            "::juce::JSON::parse",
                                                            "::juce::Typeface::createSystemTypefaceFor"))
                                        .bind("function")),
                             inPaint)
                        .bind("expensive"),
                    this);

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::Image")),
                                                                       unless(isCopyConstructor()),
                                                                       unless(isMoveConstructor()),
                                                                       unless(isDefaultConstructor()))),
                                     inPaint)
                        .bind("expensive"),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(ofClass(isSameOrDerivedFrom(hasAnyName("::juce::File",
                                                                                                  "::juce::FileInputStream"))))
                                                 .bind("function")),
                                      inPaint)
                        .bind("expensive"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* expensive = result.Nodes.getNodeAs<Expr>("expensive");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                const auto* paint = result.Nodes.getNodeAs<CXXMethodDecl>("paint");

                if (utils::isInMacroBody(expensive->getBeginLoc(), *result.SourceManager))
                    return;

                const auto description = function != nullptr
                                           ? "calling '" + utils::qualifiedName(*function) + "'"
                                           : std::string{ "allocating a juce::Image" };

                diag(expensive->getBeginLoc(),
                     "%0 from %1 will impact UI performance. Do it once and "
                     "cache the result")
                    << description
                    << paint;
            }
        };
    } // namespace expensive_paint

    using expensive_paint::ExpensivePaintCheck;
} // namespace clang::tidy::juce
