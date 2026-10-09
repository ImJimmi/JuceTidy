#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/SmallPtrSet.h>

namespace clang::tidy::juce
{
    namespace member_access_order
    {
        class MemberAccessOrderCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isImplicit()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                const AccessSpecDecl* currentSection = nullptr;
                llvm::SmallPtrSet<const Decl*, 4> reportedSections;
                auto highestRank = 0;

                for (const auto* decl : record->decls())
                {
                    if (const auto* section = dyn_cast<AccessSpecDecl>(decl))
                    {
                        if (!section->getLocation().isMacroID())
                            currentSection = section;

                        continue;
                    }

                    if (!isOrderedMember(*decl))
                        continue;

                    const auto rank = getRank(decl->getAccess());

                    if (rank < highestRank)
                    {
                        const Decl* reported = currentSection != nullptr
                                                 ? static_cast<const Decl*>(currentSection)
                                                 : decl;

                        if (reportedSections.insert(reported).second)
                        {
                            diag(reported->getLocation(),
                                 "%0 members should be declared before %1 "
                                 "members")
                                << getSpelling(rank)
                                << getSpelling(highestRank);
                        }
                    }

                    highestRank = std::max(highestRank, rank);
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            [[nodiscard]] static int getRank(AccessSpecifier access)
            {
                switch (access)
                {
                case AS_public:
                    return 0;
                case AS_protected:
                    return 1;
                default:
                    return 2;
                }
            }

            [[nodiscard]] static StringRef getSpelling(int rank)
            {
                switch (rank)
                {
                case 0:
                    return "public";
                case 1:
                    return "protected";
                default:
                    return "private";
                }
            }

            [[nodiscard]] static bool isOrderedMember(const Decl& decl)
            {
                if (decl.isImplicit() || decl.getLocation().isMacroID())
                    return false;

                if (const auto* templateDecl = dyn_cast<TemplateDecl>(&decl))
                    return !isa_and_nonnull<TypeDecl>(templateDecl->getTemplatedDecl());

                return !isa<TypeDecl,
                            FriendDecl,
                            StaticAssertDecl,
                            UsingDecl,
                            UsingShadowDecl,
                            EmptyDecl>(decl);
            }
        };
    } // namespace member_access_order

    using member_access_order::MemberAccessOrderCheck;
} // namespace clang::tidy::juce
