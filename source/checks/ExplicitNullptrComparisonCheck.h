#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace explicit_nullptr_comparison
    {
        class ExplicitNullptrComparisonCheck : public JuceTidyCheck
        {
        public:
            ExplicitNullptrComparisonCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawSmartPointers{ Options.get("SmartPointers", defaultSmartPointers) }
                , smartPointers{ utils::parseQualifiedNames(rawSmartPointers) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "SmartPointers", rawSmartPointers);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const auto smartPointerToBool = cxxMemberCallExpr(callee(cxxConversionDecl(returns(booleanType()),
                                                                                           ofClass(cxxRecordDecl(utils::isSameOrDerivedFromAnyOf(smartPointers))))));

                finder->addMatcher(
                    implicitCastExpr(unless(isInTemplateInstantiation()),
                                     unless(hasParent(explicitCastExpr())),
                                     hasCastKind(CK_UserDefinedConversion),
                                     has(smartPointerToBool))
                        .bind("conversion"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* conversion = result.Nodes.getNodeAs<ImplicitCastExpr>("conversion");
                auto& context = *result.Context;
                const auto& sourceManager = *result.SourceManager;
                const auto& langOptions = context.getLangOpts();

                if (isConditionVariable(*conversion, context))
                    return;

                const auto& object = getObject(*conversion);
                const Expr* pointer = conversion;

                if (!utils::getFileRange(pointer->getSourceRange(), sourceManager, langOptions))
                    pointer = object.IgnoreParenImpCasts();

                const auto pointerText = utils::getSourceText(pointer->getSourceRange(), sourceManager, langOptions);

                if (!pointerText)
                    return;

                const auto* negation = getNegation(*conversion, context);
                const Expr& replaced = negation != nullptr
                                         ? static_cast<const Expr&>(*negation)
                                         : *pointer;
                const auto replacedRange = utils::getFileRange(replaced.getSourceRange(), sourceManager, langOptions);

                auto comparison = (needsParentheses(object)
                                       ? "(" + pointerText->str() + ")"
                                       : pointerText->str())
                                + (negation != nullptr
                                       ? " == nullptr"
                                       : " != nullptr");

                if (const auto parent = utils::getSingleParent(replaced, context);
                    parent && bindsTighterThanEquality(*parent))
                {
                    comparison = "(" + comparison + ")";
                }

                const auto diagnostic = diag(replaced.getBeginLoc(),
                                             "avoid implicit bool conversions "
                                             "with %0. Explicitly compare with "
                                             "nullptr")
                                     << object.getType().getUnqualifiedType();

                if (replacedRange)
                    diagnostic << FixItHint::CreateReplacement(*replacedRange, comparison);
            }

        private:
            static inline constexpr auto defaultSmartPointers =
                "::std::unique_ptr;"
                "::std::shared_ptr;"
                "::std::function";

            template <typename... Statements>
            [[nodiscard]] static bool isConditionVariableOf(const DynTypedNode& node, const Decl* variable)
            {
                return ((node.get<Statements>() != nullptr && node.get<Statements>()->getConditionVariable() == variable) || ...);
            }

            [[nodiscard]] static const Expr& getObject(const ImplicitCastExpr& conversion)
            {
                return *cast<CXXMemberCallExpr>(conversion.getSubExpr()->IgnoreImplicit())->getImplicitObjectArgument();
            }

            [[nodiscard]] static bool isConditionVariable(const ImplicitCastExpr& conversion, ASTContext& context)
            {
                const auto* reference = dyn_cast<DeclRefExpr>(getObject(conversion).IgnoreImplicit());

                if (reference == nullptr)
                    return false;

                return llvm::any_of(context.getParents(conversion),
                                    [&](const DynTypedNode& parent) {
                                        return isConditionVariableOf<IfStmt, WhileStmt, ForStmt, SwitchStmt>(parent, reference->getDecl());
                                    });
            }

            [[nodiscard]] static const UnaryOperator* getNegation(const Expr& conversion, ASTContext& context)
            {
                const auto parent = utils::getSingleParent(conversion, context);

                if (!parent)
                    return nullptr;

                const auto* negation = parent->get<UnaryOperator>();

                if (negation == nullptr
                    || negation->getOpcode() != UO_LNot
                    || negation->getOperatorLoc().isMacroID())
                {
                    return nullptr;
                }

                return negation;
            }

            [[nodiscard]] static bool bindsTighterThanEquality(const DynTypedNode& node)
            {
                if (node.get<UnaryOperator>() != nullptr)
                    return true;

                if (const auto* binary = node.get<BinaryOperator>())
                {
                    return binary->isMultiplicativeOp()
                        || binary->isAdditiveOp()
                        || binary->isShiftOp()
                        || binary->isRelationalOp()
                        || binary->isEqualityOp()
                        || binary->getOpcode() == BO_Cmp;
                }

                return false;
            }

            [[nodiscard]] static bool needsParentheses(const Expr& pointer)
            {
                return isa<BinaryOperator,
                           ConditionalOperator,
                           BinaryConditionalOperator>(pointer.IgnoreImpCasts());
            }

            const StringRef rawSmartPointers;
            const std::vector<std::string> smartPointers;
        };
    } // namespace explicit_nullptr_comparison

    using explicit_nullptr_comparison::ExplicitNullptrComparisonCheck;
} // namespace clang::tidy::juce
