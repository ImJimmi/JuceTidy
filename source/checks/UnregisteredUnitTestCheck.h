#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallPtrSet.h>

#include <cctype>

namespace clang::tidy::juce
{
    namespace unregistered_unit_test
    {
        class UnregisteredUnitTestCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto unitTest = cxxRecordDecl(isSameOrDerivedFrom(hasName("::juce::UnitTest")));

                finder->addMatcher(
                    cxxRecordDecl(isDefinition(),
                                  isExpansionInMainFile(),
                                  unitTest,
                                  unless(isTemplateInstantiation()))
                        .bind("test"),
                    this);

                finder->addMatcher(
                    cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(unitTest.bind("constructed"))))),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* constructed = result.Nodes.getNodeAs<CXXRecordDecl>("constructed"))
                {
                    instantiated.insert(constructed->getCanonicalDecl());
                    return;
                }

                const auto* test = result.Nodes.getNodeAs<CXXRecordDecl>("test");

                for (const auto& base : test->bases())
                    if (const auto* baseRecord = base.getType()->getAsCXXRecordDecl())
                        bases.insert(baseRecord->getCanonicalDecl());

                if (test->getDescribedClassTemplate() == nullptr && !test->isAbstract() && !test->getLocation().isMacroID())
                    tests.insert(test);
            }

            void onEndOfTranslationUnit() override
            {
                for (const auto* test : tests)
                {
                    if (instantiated.contains(test->getCanonicalDecl()) || bases.contains(test->getCanonicalDecl()))
                        continue;

                    auto instanceName = test->getNameAsString();
                    instanceName.front() = static_cast<char>(std::tolower(static_cast<unsigned char>(instanceName.front())));

                    diag(test->getLocation(),
                         "%0 is never instantiated, so its tests will never be "
                         "run. Add a static instance, such as 'static %1 %2;'")
                        << test
                        << test->getName()
                        << instanceName;
                }

                tests.clear();
                instantiated.clear();
                bases.clear();
            }

        private:
            llvm::SetVector<const CXXRecordDecl*> tests;
            llvm::SmallPtrSet<const Decl*, 16> instantiated;
            llvm::SmallPtrSet<const Decl*, 16> bases;
        };
    } // namespace unregistered_unit_test

    using unregistered_unit_test::UnregisteredUnitTestCheck;
} // namespace clang::tidy::juce
