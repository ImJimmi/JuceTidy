#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace explicit_base_access
    {
        class ExplicitBaseAccessCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                const auto* const defaultAccess = record->isClass() ? "private" : "public";

                for (const auto& base : record->bases())
                {
                    if (base.getAccessSpecifierAsWritten() != AS_none || base.getBeginLoc().isMacroID())
                        continue;

                    diag(base.getBeginLoc(),
                         "inheritance from %0 is implicitly %1. Make it "
                         "explicit")
                        << base.getType()
                        << defaultAccess
                        << FixItHint::CreateInsertion(base.getBeginLoc(), std::string{ defaultAccess } + " ");
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }
        };
    } // namespace explicit_base_access

    using explicit_base_access::ExplicitBaseAccessCheck;
} // namespace clang::tidy::juce
