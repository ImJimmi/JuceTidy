#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace template_parameter_name
    {
        class TemplateParameterNameCheck : public JuceTidyCheck
        {
        public:
            TemplateParameterNameCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , minimumLength{ Options.get("MinimumLength", 2u) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "MinimumLength", minimumLength);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    namedDecl(anyOf(templateTypeParmDecl(),
                                    nonTypeTemplateParmDecl(),
                                    templateTemplateParmDecl()),
                              unless(isImplicit()))
                        .bind("parameter"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* parameter = result.Nodes.getNodeAs<NamedDecl>("parameter");
                const auto name = parameter->getName();

                if (name.empty()
                    || name.size() >= minimumLength
                    || parameter->getLocation().isMacroID())
                {
                    return;
                }

                diag(parameter->getLocation(),
                     "template parameter %0 should have a descriptive name of "
                     "at least %1 characters")
                    << parameter
                    << minimumLength;
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            const unsigned minimumLength;
        };
    } // namespace template_parameter_name

    using template_parameter_name::TemplateParameterNameCheck;
} // namespace clang::tidy::juce
