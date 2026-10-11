#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace missing_accessibility_handler
    {
        class MissingAccessibilityHandlerCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 unless(isTemplateInstantiation()),
                                                 isSameOrDerivedFrom(hasName("::juce::Component")),
                                                 unless(isSameOrDerivedFrom(hasName("::juce::AudioProcessorEditor"))),
                                                 has(cxxMethodDecl(hasAnyName("mouseDown",
                                                                              "mouseUp",
                                                                              "mouseDrag",
                                                                              "mouseDoubleClick"),
                                                                   isOverride())))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");

                if (record->getLocation().isMacroID()
                    || utils::findAccessibilityHandlerOverride(*record) != nullptr
                    || isHiddenFromAccessibility(*record, *result.Context))
                {
                    return;
                }

                diag(record->getLocation(),
                     "%0 handles mouse input but doesn't override "
                     "'createAccessibilityHandler()', so screen readers won't "
                     "know what it is or be able to interact with it")
                    << record;
            }

        private:
            [[nodiscard]] static bool isHiddenFromAccessibility(const CXXRecordDecl& record, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto statements = utils::getClassStatements(record);

                if (!statements)
                    return false;

                const auto hide = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("setAccessible"))),
                                                    on(cxxThisExpr()),
                                                    hasArgument(0, ignoringParenImpCasts(cxxBoolLiteral(equals(false)))));

                return llvm::any_of(*statements, [&](const Stmt* statement) {
                    return utils::findFirst(hide, *statement, context) != nullptr;
                });
            }
        };
    } // namespace missing_accessibility_handler

    using missing_accessibility_handler::MissingAccessibilityHandlerCheck;
} // namespace clang::tidy::juce
