#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringExtras.h>

namespace clang::tidy::juce
{
    namespace relative_file_path
    {
        class RelativeFilePathCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto literal = stringLiteral().bind("path");

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::File")),
                                                                       unless(isCopyConstructor()),
                                                                       unless(isMoveConstructor()))),
                                     hasArgument(0,
                                                 ignoringImplicit(anyOf(ignoringParenImpCasts(literal),
                                                                        cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::String")))),
                                                                                         hasArgument(0, ignoringParenImpCasts(literal)))))))
                        .bind("file"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* file = result.Nodes.getNodeAs<CXXConstructExpr>("file");
                const auto* path = result.Nodes.getNodeAs<StringLiteral>("path");
                const auto& sourceManager = *result.SourceManager;
                const auto& langOptions = result.Context->getLangOpts();

                if (!path->isOrdinary()
                    || utils::isInMacroBody(path->getBeginLoc(), sourceManager)
                    || isAbsolutePath(path->getString()))
                {
                    return;
                }

                const auto diagnostic = diag(path->getBeginLoc(),
                                             "'%0' needs to be an absolute "
                                             "path")
                                     << path->getString();

                const auto argumentRange = file->getArg(0)->getSourceRange();
                const auto range = utils::getFileRange(argumentRange, sourceManager, langOptions);
                const auto argument = utils::getSourceText(argumentRange, sourceManager, langOptions);

                if (range && argument)
                {
                    diagnostic << FixItHint::CreateReplacement(*range,
                                                               "juce::File::getCurrentWorkingDirectory().getChildFile(" + argument->str() + ")");
                }
            }

        private:
            [[nodiscard]] static bool isAbsolutePath(StringRef path)
            {
                if (path.empty()
                    || path.starts_with("/")
                    || path.starts_with("~")
                    || path.starts_with("\\\\"))
                {
                    return true;
                }

                const auto driveSeparator = path.drop_front().take_front(2);

                return path.size() >= 3
                    && llvm::isAlpha(path.front())
                    && (driveSeparator == ":\\" || driveSeparator == ":/");
            }
        };
    } // namespace relative_file_path

    using relative_file_path::RelativeFilePathCheck;
} // namespace clang::tidy::juce
