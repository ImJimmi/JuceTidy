#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace explicit_unsigned_int
    {
        class ExplicitUnsignedIntCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(typeLoc(loc(qualType(hasUnqualifiedDesugaredType(builtinType()))))
                                       .bind("type"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto type = result.Nodes.getNodeAs<TypeLoc>("type")->getUnqualifiedLoc().getAs<BuiltinTypeLoc>();

                if (!type
                    || !type.getTypePtr()->isSpecificBuiltinType(BuiltinType::UInt)
                    || type.getBeginLoc().isMacroID())
                {
                    return;
                }

                const auto range = utils::getFileRange(type.getSourceRange(), *result.SourceManager, result.Context->getLangOpts());

                if (!range
                    || Lexer::getSourceText(*range, *result.SourceManager, result.Context->getLangOpts()) != "unsigned")
                {
                    return;
                }

                diag(type.getBeginLoc(),
                     "explicitly use 'unsigned int', not 'unsigned'")
                    << FixItHint::CreateReplacement(*range, "unsigned int");
            }
        };
    } // namespace explicit_unsigned_int

    using explicit_unsigned_int::ExplicitUnsignedIntCheck;
} // namespace clang::tidy::juce
