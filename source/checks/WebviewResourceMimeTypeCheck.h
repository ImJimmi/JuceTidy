#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace webview_resource_mime_type
    {
        class WebviewResourceMimeTypeCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    initListExpr(unless(isInTemplateInstantiation()),
                                 hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(cxxRecordDecl(hasName("::juce::WebBrowserComponent::Resource")))))))
                        .bind("resource"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* resource = result.Nodes.getNodeAs<InitListExpr>("resource");

                if ((resource->isSyntacticForm() && resource->getSemanticForm() != nullptr)
                    || utils::isInMacroBody(resource->getBeginLoc(), *result.SourceManager))
                {
                    return;
                }

                if (resource->getNumInits() > 1 && !isEmptyString(*resource->getInit(1)))
                    return;

                diag(resource->getBeginLoc(),
                     "this resource has no MIME type, so the browser may refuse "
                     "to load it. Set 'mimeType', for example to "
                     "\"text/javascript\"");
            }

        private:
            [[nodiscard]] static bool isEmptyString(const Expr& expression)
            {
                const auto* value = expression.IgnoreImplicit();

                if (isa<ImplicitValueInitExpr>(value))
                    return true;

                if (const auto* construction = dyn_cast<CXXConstructExpr>(value);
                    construction != nullptr && construction->getNumArgs() == 0)
                {
                    return true;
                }

                const auto* literal = utils::getStringLiteral(expression);
                return literal != nullptr && literal->getLength() == 0;
            }
        };
    } // namespace webview_resource_mime_type

    using webview_resource_mime_type::WebviewResourceMimeTypeCheck;
} // namespace clang::tidy::juce
