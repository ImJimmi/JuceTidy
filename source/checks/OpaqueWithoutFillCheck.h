#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace opaque_without_fill
    {
        class OpaqueWithoutFillCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("setOpaque"),
                                                           ofClass(hasName("::juce::Component")))),
                                      hasArgument(0, ignoringParenImpCasts(cxxBoolLiteral(equals(true)))),
                                      on(cxxThisExpr()),
                                      forCallable(cxxMethodDecl(ofClass(cxxRecordDecl().bind("owner")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");
                const auto* paint = findPaint(*owner);

                if (paint == nullptr)
                {
                    diag(call->getExprLoc(),
                         "%0 is opaque but doesn't override 'paint()'")
                        << owner;

                    return;
                }

                const FunctionDecl* definition = nullptr;

                if (!paint->hasBody(definition)
                    || definition == nullptr
                    || definition->getNumParams() != 1)
                {
                    return;
                }

                if (mayFillEverything(*definition->getBody(), *definition->getParamDecl(0), *result.Context))
                    return;

                diag(call->getExprLoc(),
                     "%0 is opaque but its 'paint()' method doesn't fill the "
                     "whole component. Call 'g.fillAll()' somewhere in "
                     "'paint()'")
                    << owner;
            }

        private:
            [[nodiscard]] static const CXXMethodDecl* findPaint(const CXXRecordDecl& record)
            {
                static const std::vector<std::string> paintMethod{ "juce::Component::paint" };

                for (const auto* method : record.methods())
                {
                    if (utils::isOrOverridesAnyOf(*method, paintMethod) && method->size_overridden_methods() > 0)
                        return method;
                }

                for (const auto& base : record.bases())
                {
                    if (const auto* baseRecord = utils::getRecord(base.getType());
                        baseRecord != nullptr && utils::qualifiedName(*baseRecord) != "juce::Component")
                    {
                        if (const auto* definition = baseRecord->getDefinition();
                            definition != nullptr)
                        {
                            if (const auto* method = findPaint(*definition))
                                return method;
                        }
                    }
                }

                return nullptr;
            }

            [[nodiscard]] static bool mayFillEverything(const Stmt& body,
                                                        const ParmVarDecl& graphics,
                                                        ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto fill = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("fillAll",
                                                                                    "fillRect",
                                                                                    "drawImage",
                                                                                    "drawImageAt",
                                                                                    "drawImageWithin",
                                                                                    "drawImageTransformed"),
                                                                         ofClass(hasName("::juce::Graphics")))));
                const auto passesGraphicsOn = callExpr(hasAnyArgument(ignoringParenImpCasts(declRefExpr(to(equalsNode(&graphics))))),
                                                       unless(callee(functionDecl(matchesName("^::juce::Graphics::")))));

                return utils::findFirst(stmt(anyOf(fill, passesGraphicsOn)), body, context) != nullptr;
            }
        };
    } // namespace opaque_without_fill

    using opaque_without_fill::OpaqueWithoutFillCheck;
} // namespace clang::tidy::juce
