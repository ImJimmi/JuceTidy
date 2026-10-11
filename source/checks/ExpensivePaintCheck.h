#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang::tidy::juce
{
    namespace expensive_paint
    {
        class ExpensivePaintCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                const std::vector<std::string> paintMethods{
                    "juce::Component::paint",
                    "juce::Component::paintOverChildren",
                };
                const auto paint = cxxMethodDecl(utils::isOrOverridesAnyOf(paintMethods)).bind("paint");
                const auto inPaint = forCallable(paint);
                const auto gradient = cxxRecordDecl(hasName("::juce::ColourGradient"));

                finder->addMatcher(
                    callExpr(unless(isInTemplateInstantiation()),
                             callee(functionDecl(hasAnyName("::juce::ImageFileFormat::loadFrom",
                                                            "::juce::Drawable::createFromImageData",
                                                            "::juce::Drawable::createFromImageFile",
                                                            "::juce::Drawable::createFromSVG",
                                                            "::juce::Drawable::createFromSVGFile",
                                                            "::juce::parseXML",
                                                            "::juce::XmlDocument::parse",
                                                            "::juce::JSON::parse",
                                                            "::juce::Typeface::createSystemTypefaceFor"))
                                        .bind("function")),
                             inPaint)
                        .bind("expensive"),
                    this);

                finder->addMatcher(
                    cxxConstructExpr(unless(isInTemplateInstantiation()),
                                     hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::Image")),
                                                                       unless(isCopyConstructor()),
                                                                       unless(isMoveConstructor()),
                                                                       unless(isDefaultConstructor()))),
                                     inPaint)
                        .bind("expensive"),
                    this);

                finder->addMatcher(
                    cxxMemberCallExpr(unless(isInTemplateInstantiation()),
                                      callee(cxxMethodDecl(ofClass(isSameOrDerivedFrom(hasAnyName("::juce::File",
                                                                                                  "::juce::FileInputStream"))))
                                                 .bind("function")),
                                      inPaint)
                        .bind("expensive"),
                    this);

                finder->addMatcher(
                    varDecl(unless(isInstantiated()),
                            unless(parmVarDecl()),
                            hasLocalStorage(),
                            hasType(cxxRecordDecl(hasAnyName("::juce::Path", "::juce::ColourGradient"))),
                            hasDeclContext(paint))
                        .bind("built"),
                    this);

                finder->addMatcher(
                    expr(unless(isInTemplateInstantiation()),
                         anyOf(cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(gradient),
                                                                                  unless(isCopyConstructor()),
                                                                                  unless(isMoveConstructor()),
                                                                                  unless(isDefaultConstructor())))),
                               callExpr(callee(cxxMethodDecl(isStaticStorageClass(), ofClass(gradient))))),
                         unless(hasAncestor(varDecl(hasType(gradient)))),
                         inPaint)
                        .bind("temporary"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                if (const auto* built = result.Nodes.getNodeAs<VarDecl>("built"))
                {
                    checkBuiltVariable(*built, *result.Nodes.getNodeAs<CXXMethodDecl>("paint"), result);
                    return;
                }

                if (const auto* temporary = result.Nodes.getNodeAs<Expr>("temporary"))
                {
                    if (!utils::isInMacroBody(temporary->getBeginLoc(), *result.SourceManager)
                        && isFixed(*temporary, nullptr, *result.Context))
                    {
                        diagnoseFixed(temporary->getBeginLoc(),
                                      "this juce::ColourGradient",
                                      *result.Nodes.getNodeAs<CXXMethodDecl>("paint"));
                    }

                    return;
                }

                const auto* expensive = result.Nodes.getNodeAs<Expr>("expensive");
                const auto* function = result.Nodes.getNodeAs<FunctionDecl>("function");
                const auto* paint = result.Nodes.getNodeAs<CXXMethodDecl>("paint");

                if (utils::isInMacroBody(expensive->getBeginLoc(), *result.SourceManager))
                    return;

                const auto description = function != nullptr
                                           ? "calling '" + utils::qualifiedName(*function) + "'"
                                           : std::string{ "allocating a juce::Image" };

                diag(expensive->getBeginLoc(),
                     "%0 from %1 will impact UI performance. Do it once and "
                     "cache the result")
                    << description
                    << paint;
            }

        private:
            void diagnoseFixed(SourceLocation location, const std::string& description, const CXXMethodDecl& paint)
            {
                diag(location,
                     "%0 is built from the same values every time %1 is "
                     "called. Build it once and store it as a member")
                    << description
                    << &paint;
            }

            void checkBuiltVariable(const VarDecl& variable,
                                    const CXXMethodDecl& paint,
                                    const ast_matchers::MatchFinder::MatchResult& result)
            {
                using namespace clang::ast_matchers;

                auto& context = *result.Context;
                const auto* init = variable.getInit();
                const auto* construction = init != nullptr
                                             ? dyn_cast<CXXConstructExpr>(init->IgnoreImplicit())
                                             : nullptr;
                auto isBuilt = init != nullptr
                            && (construction == nullptr
                                || construction->getNumArgs() > 0);

                if (paint.getBody() == nullptr
                    || utils::isInMacroBody(variable.getLocation(), *result.SourceManager)
                    || (init != nullptr && !isFixed(*init, &variable, context)))
                {
                    return;
                }

                for (const auto& nodes : match(findAll(declRefExpr(to(varDecl(equalsNode(&variable)))).bind("use")),
                                               *paint.getBody(),
                                               context))
                {
                    const auto use = classifyUse(*nodes.getNodeAs<DeclRefExpr>("use"), variable, context);

                    if (use == Use::unknown)
                        return;

                    isBuilt = isBuilt || use == Use::build;
                }

                if (isBuilt)
                    diagnoseFixed(variable.getLocation(), "'" + variable.getNameAsString() + "'", paint);
            }

            enum class Use
            {
                read,
                build,
                unknown,
            };

            [[nodiscard]] static Use classifyUse(const DeclRefExpr& use, const VarDecl& variable, ASTContext& context)
            {
                const Expr* current = &use;
                auto parent = utils::getSingleParent(*current, context);

                while (parent.has_value()
                       && parent->get<Expr>() != nullptr
                       && isa<ImplicitCastExpr, ParenExpr>(parent->get<Expr>()))
                {
                    current = parent->get<Expr>();
                    parent = utils::getSingleParent(*current, context);
                }

                if (!parent.has_value())
                    return Use::unknown;

                if (const auto* member = parent->get<MemberExpr>())
                {
                    const auto memberParent = utils::getSingleParent(*member, context);
                    const auto* call = memberParent.has_value()
                                         ? memberParent->get<CXXMemberCallExpr>()
                                         : nullptr;

                    if (call == nullptr || call->getMethodDecl() == nullptr)
                        return Use::unknown;

                    if (call->getMethodDecl()->isConst())
                        return Use::read;

                    return llvm::all_of(call->arguments(),
                                        [&](const Expr* argument) {
                                            return isFixed(*argument, &variable, context);
                                        })
                             ? Use::build
                             : Use::unknown;
                }

                if (const auto* assignment = parent->get<CXXOperatorCallExpr>();
                    assignment != nullptr
                    && assignment->getOperator() == OO_Equal
                    && assignment->getArg(0)->IgnoreParenImpCasts() == &use)
                {
                    return isFixed(*assignment->getArg(1), &variable, context)
                             ? Use::build
                             : Use::unknown;
                }

                const auto* call = parent->get<Expr>();
                const auto* callee = call != nullptr
                                       ? getCallee(*call)
                                       : nullptr;

                if (callee == nullptr)
                    return Use::unknown;

                const auto arguments = utils::getCallArguments(*call);
                const auto* argument = llvm::find(arguments, current);
                auto index = static_cast<unsigned int>(std::distance(arguments.begin(), argument));

                if (isa<CXXOperatorCallExpr>(call) && isa<CXXMethodDecl>(callee))
                {
                    if (index == 0)
                        return Use::unknown;

                    --index;
                }

                if (argument == arguments.end() || index >= callee->getNumParams())
                    return Use::unknown;

                const auto type = callee->getParamDecl(index)->getType();

                return !type->isPointerType()
                            && (!type->isReferenceType() || type.getNonReferenceType().isConstQualified())
                         ? Use::read
                         : Use::unknown;
            }

            [[nodiscard]] static const FunctionDecl* getCallee(const Expr& call)
            {
                if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
                    return construction->getConstructor();

                if (const auto* callExpression = dyn_cast<CallExpr>(&call))
                    return callExpression->getDirectCallee();

                return nullptr;
            }

            [[nodiscard]] static bool isFixed(const Stmt& statement, const VarDecl* self, ASTContext& context, int depth = 0)
            {
                using namespace clang::ast_matchers;

                if (depth > 4 || utils::findFirst(cxxThisExpr(), statement, context) != nullptr)
                    return false;

                return llvm::all_of(match(findAll(declRefExpr().bind("reference")), statement, context),
                                    [&](const BoundNodes& nodes) {
                                        const auto* declaration = nodes.getNodeAs<DeclRefExpr>("reference")->getDecl();
                                        const auto* variable = dyn_cast<VarDecl>(declaration);

                                        if (variable == nullptr)
                                            return !isa<BindingDecl>(declaration);

                                        if (variable == self)
                                            return true;

                                        if (isa<ParmVarDecl>(variable)
                                            || !variable->getType().isConstQualified())
                                        {
                                            return false;
                                        }

                                        if (!variable->hasLocalStorage() && !variable->isStaticLocal())
                                            return true;

                                        return variable->getInit() != nullptr
                                            && isFixed(*variable->getInit(), self, context, depth + 1);
                                    });
            }
        };
    } // namespace expensive_paint

    using expensive_paint::ExpensivePaintCheck;
} // namespace clang::tidy::juce
