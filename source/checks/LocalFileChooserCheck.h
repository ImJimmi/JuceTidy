#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace local_file_chooser
    {
        class LocalFileChooserCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto fileChooser = cxxRecordDecl(hasName("::juce::FileChooser"));

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(hasName("launchAsync"),
                                                           ofClass(fileChooser))),
                                      on(declRefExpr(to(varDecl(hasAutomaticStorageDuration(),
                                                                hasType(fileChooser))
                                                            .bind("chooser")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* chooser = result.Nodes.getNodeAs<VarDecl>("chooser");

                diag(call->getExprLoc(),
                     "the file chooser will close as soon as %0 goes out of "
                     "scope. Store it in a member so it persists")
                    << chooser;
            }
        };
    } // namespace local_file_chooser

    using local_file_chooser::LocalFileChooserCheck;
} // namespace clang::tidy::juce
