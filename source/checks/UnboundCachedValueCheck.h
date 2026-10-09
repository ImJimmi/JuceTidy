#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace unbound_cached_value
    {
        class UnboundCachedValueCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(cxxRecordDecl(isDefinition(),
                                                 unless(isLambda()),
                                                 has(fieldDecl()))
                                       .bind("record"),
                                   this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* record = result.Nodes.getNodeAs<CXXRecordDecl>("record");
                std::vector<const FieldDecl*> candidates;

                for (const auto* field : record->fields())
                {
                    if (isCachedValue(*field)
                        && !field->getLocation().isMacroID()
                        && !isConstructedWithArguments(field->getInClassInitializer()))
                    {
                        candidates.push_back(field);
                    }
                }

                if (candidates.empty())
                    return;

                const auto statements = utils::getClassStatements(*record);

                if (!statements)
                    return;

                for (const auto* field : candidates)
                {
                    if (isBoundInConstructor(*field, *record)
                        || isBoundInBody(*field, *statements, *result.Context))
                    {
                        continue;
                    }

                    diag(field->getLocation(),
                         "%0 is never attached to a ValueTree. Call "
                         "'%1.referTo(...) before using it'")
                        << field
                        << field->getName();
                }
            }

            [[nodiscard]] std::optional<TraversalKind> getCheckTraversalKind() const override
            {
                return TK_IgnoreUnlessSpelledInSource;
            }

        private:
            [[nodiscard]] static bool isCachedValue(const FieldDecl& field)
            {
                const auto* record = utils::getRecord(field.getType());
                return record != nullptr && utils::qualifiedName(*record) == "juce::CachedValue";
            }

            [[nodiscard]] static bool isConstructedWithArguments(const Expr* initialiser)
            {
                const auto* construction = initialiser != nullptr
                                             ? dyn_cast<CXXConstructExpr>(initialiser->IgnoreImplicit())
                                             : nullptr;

                return construction != nullptr
                    && construction->getNumArgs() > 0
                    && !construction->getConstructor()->isCopyOrMoveConstructor();
            }

            [[nodiscard]] static bool isBoundInConstructor(const FieldDecl& field, const CXXRecordDecl& record)
            {
                for (const auto* constructor : record.ctors())
                {
                    const auto* definition = dyn_cast_or_null<CXXConstructorDecl>(constructor->getDefinition());

                    if (definition == nullptr)
                        continue;

                    for (const auto* initialiser : definition->inits())
                    {
                        if (initialiser->getMember() == &field
                            && initialiser->isWritten()
                            && isConstructedWithArguments(initialiser->getInit()))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            [[nodiscard]] static bool isBoundInBody(const FieldDecl& field,
                                                    ArrayRef<const Stmt*> statements,
                                                    ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto references = [&field](const Expr* expression) {
                    return expression != nullptr && utils::getReferencedField(*expression) == &field;
                };

                for (const auto* statement : statements)
                {
                    for (const auto& nodes : match(findAll(cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("referTo",
                                                                                                             "referToWithDefault"))))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (references(nodes.getNodeAs<CXXMemberCallExpr>("call")->getImplicitObjectArgument()))
                            return true;
                    }

                    for (const auto& nodes : match(findAll(expr(anyOf(callExpr(unless(cxxOperatorCallExpr())),
                                                                      cxxConstructExpr()))
                                                               .bind("call")),
                                                   *statement,
                                                   context))
                    {
                        if (llvm::any_of(utils::getCallArguments(*nodes.getNodeAs<Expr>("call")), references))
                            return true;
                    }
                }

                return false;
            }
        };
    } // namespace unbound_cached_value

    using unbound_cached_value::UnboundCachedValueCheck;
} // namespace clang::tidy::juce
