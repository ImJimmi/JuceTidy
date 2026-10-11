#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace buffered_parent
    {
        class BufferedParentCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMemberCallExpr(
                        unless(isInTemplateInstantiation()),
                        callee(cxxMethodDecl(hasName("::juce::Component::setBufferedToImage"))),
                        unless(hasArgument(0, ignoringParenImpCasts(cxxBoolLiteral(equals(false))))),
                        forCallable(cxxMethodDecl(ofClass(cxxRecordDecl(unless(isLambda()))
                                                              .bind("owner")))))
                        .bind("call"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                using namespace clang::ast_matchers;

                const auto* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
                const auto* owner = result.Nodes.getNodeAs<CXXRecordDecl>("owner");
                auto& context = *result.Context;
                const auto target = getTarget(*call);

                if (!target.has_value()
                    || utils::isInMacroBody(call->getExprLoc(), *result.SourceManager))
                {
                    return;
                }

                const auto addChild = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("addAndMakeVisible",
                                                                                        "addChildComponent"),
                                                                             ofClass(hasName("::juce::Component")))))
                                          .bind("add");

                for (const auto* statement : getClassStatements(*owner))
                {
                    for (const auto& nodes : match(findAll(addChild),
                                                   *statement,
                                                   context))
                    {
                        const auto* add = nodes.getNodeAs<CXXMemberCallExpr>("add");

                        if (getTarget(*add) != target)
                            continue;

                        const auto* component = *target != nullptr
                                                  ? static_cast<const NamedDecl*>(*target)
                                                  : owner;

                        diag(call->getExprLoc(),
                             "%0 has child components so its cached image will "
                             "be repainted whenever one of its children is "
                             "repainted. Use a child component to display this "
                             "component's background graphics instead (see "
                             "https://forum.juce.com/t/the-big-list-of-juce-tips-and-tricks-from-n00b-to-pro/62193/11 "
                             "and https://youtu.be/0n9x6R0fheo?si=v4wZkNxRKJurLUvr&t=1222 "
                             "for further explanations)")
                            << component;

                        return;
                    }
                }
            }

        private:
            [[nodiscard]] static std::optional<const FieldDecl*> getTarget(const CXXMemberCallExpr& call)
            {
                const auto* object = call.getImplicitObjectArgument();

                if (object == nullptr)
                    return std::nullopt;

                if (isa<CXXThisExpr>(object->IgnoreParenImpCasts()))
                    return nullptr;

                if (const auto* field = utils::getReferencedField(*object))
                    return field;

                return std::nullopt;
            }

            [[nodiscard]] static std::vector<const Stmt*> getClassStatements(const CXXRecordDecl& record)
            {
                std::vector<const Stmt*> statements;

                for (const auto* method : record.methods())
                {
                    const FunctionDecl* definition = nullptr;

                    if (method->hasBody(definition) && definition != nullptr)
                        llvm::append_range(statements, utils::getFunctionStatements(*definition));
                }

                return statements;
            }
        };
    } // namespace buffered_parent

    using buffered_parent::BufferedParentCheck;
} // namespace clang::tidy::juce
