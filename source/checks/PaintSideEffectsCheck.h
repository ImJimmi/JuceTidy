#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/DenseSet.h>

#include <map>

namespace clang::tidy::juce
{
    namespace paint_side_effects
    {
        [[nodiscard]] inline bool isLookAndFeel(const CXXRecordDecl& record)
        {
            const auto name = utils::qualifiedName(record);

            if (name == "juce::LookAndFeel"
                || (StringRef(name).starts_with("juce::") && StringRef(name).ends_with("::LookAndFeelMethods")))
            {
                return true;
            }

            const auto* definition = record.getDefinition();

            return definition != nullptr
                && llvm::any_of(definition->bases(),
                                [](const CXXBaseSpecifier& base) {
                                    const auto* baseRecord = utils::getRecord(base.getType());
                                    return baseRecord != nullptr && isLookAndFeel(*baseRecord);
                                });
        }

        // LookAndFeel methods that are given a juce::Graphics, such as
        // 'drawButtonBackground()', are called while painting.
        [[nodiscard]] inline bool drawsForLookAndFeel(const CXXMethodDecl& method)
        {
            return method.size_overridden_methods() > 0
                && llvm::any_of(method.parameters(),
                                [](const ParmVarDecl* parameter) {
                                    const auto* record = utils::getRecord(parameter->getType().getNonReferenceType());
                                    return record != nullptr && utils::qualifiedName(*record) == "juce::Graphics";
                                })
                && isLookAndFeel(*method.getParent());
        }

        AST_MATCHER(CXXMethodDecl, isLookAndFeelDrawMethod)
        {
            return drawsForLookAndFeel(Node);
        }

        class PaintSideEffectsCheck : public JuceTidyCheck
        {
        public:
            using JuceTidyCheck::JuceTidyCheck;

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    cxxMethodDecl(isDefinition(),
                                  unless(isInstantiated()),
                                  unless(isExpansionInSystemHeader()),
                                  anyOf(utils::isOrOverridesAnyOf(paintMethods),
                                        isLookAndFeelDrawMethod()))
                        .bind("entryPoint"),
                    this);
            }

            void onStartOfTranslationUnit() override
            {
                checkedFunctions.clear();
                templateSummaries.clear();
                reportedLocations.clear();
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* entryPoint = result.Nodes.getNodeAs<FunctionDecl>("entryPoint");

                if (!checkedFunctions.insert(entryPoint->getCanonicalDecl()).second)
                    return;

                std::vector<Frame> frames{
                    {
                        .function = entryPoint,
                        .callSite = nullptr,
                        .caller = std::nullopt,
                    },
                };

                for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
                    checkFrame(frameIndex, frames, *result.Context);
            }

        private:
            struct Frame
            {
                const FunctionDecl* function;
                const Expr* callSite;
                std::optional<size_t> caller;
            };

            struct TemplateSummary
            {
                std::vector<const CXXRecordDecl*> components;
                std::vector<const FunctionDecl*> callees;
            };

            static inline const std::vector<std::string> paintMethods{
                "juce::Component::paint",
                "juce::Component::paintOverChildren",
            };
            static inline const std::vector<std::string> componentNames{
                "juce::Component",
            };
            static inline constexpr auto maximumLookThroughDepth = 8;

            [[nodiscard]] static bool isEntryPoint(const FunctionDecl& function)
            {
                if (utils::isOrOverridesAnyOf(function, paintMethods))
                    return true;

                const auto* method = dyn_cast<CXXMethodDecl>(&function);
                return method != nullptr && drawsForLookAndFeel(*method);
            }

            [[nodiscard]] static bool isOpaque(const FunctionDecl& function, const SourceManager& sourceManager)
            {
                const auto name = utils::qualifiedName(function);

                return sourceManager.isInSystemHeader(sourceManager.getExpansionLoc(function.getLocation()))
                    || StringRef(name).starts_with("juce::")
                    || StringRef(name).starts_with("std::");
            }

            [[nodiscard]] static ast_matchers::internal::Matcher<Stmt> inFrame(const FunctionDecl& function)
            {
                using namespace clang::ast_matchers;

                return forCallable(functionDecl(equalsNode(&function)));
            }

            void checkFrame(size_t frameIndex, std::vector<Frame>& frames, ASTContext& context)
            {
                const auto& function = *frames.at(frameIndex).function;

                for (const auto* statement : utils::getFunctionStatements(function))
                {
                    reportSideEffects(*statement, frameIndex, frames, context);
                    followCalls(*statement, frameIndex, frames, context);
                }
            }

            void reportSideEffects(const Stmt& statement,
                                   size_t frameIndex,
                                   const std::vector<Frame>& frames,
                                   ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto& function = *frames.at(frameIndex).function;
                const auto sideEffect = cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("repaint",
                                                                                          "setBounds",
                                                                                          "setVisible",
                                                                                          "addAndMakeVisible",
                                                                                          "addChildComponent",
                                                                                          "removeChildComponent",
                                                                                          "removeAllChildren",
                                                                                          "deleteAllChildren",
                                                                                          "toFront",
                                                                                          "toBack",
                                                                                          "toBehind"),
                                                                               ofClass(isSameOrDerivedFrom(hasName("::juce::Component"))))
                                                                     .bind("method")),
                                                          inFrame(function));

                for (const auto& nodes : match(findAll(sideEffect.bind("call")), statement, context))
                {
                    const auto location = nodes.getNodeAs<CXXMemberCallExpr>("call")->getExprLoc();

                    if (!shouldReport(location, context))
                        continue;

                    diagnose(location,
                             nodes.getNodeAs<CXXMethodDecl>("method")->getName(),
                             describeContext(frameIndex, frames));

                    addCallChainNotes(frameIndex, frames);
                }
            }

            void reportConstruction(SourceLocation location,
                                    const CXXRecordDecl& component,
                                    size_t frameIndex,
                                    const std::vector<Frame>& frames,
                                    const ASTContext& context)
            {
                if (!shouldReport(location, context))
                    return;

                diag(location,
                     "constructing '%0' %1 creates a component while "
                     "painting. Create it once, before painting, as "
                     "components are costly to create and adding them to the "
                     "hierarchy while painting can crash")
                    << utils::qualifiedName(component)
                    << describeContext(frameIndex, frames);

                addCallChainNotes(frameIndex, frames);
            }

            [[nodiscard]] bool shouldReport(SourceLocation location, const ASTContext& context)
            {
                return !utils::isInMacroBody(location, context.getSourceManager())
                    && reportedLocations.insert(location).second;
            }

            void diagnose(SourceLocation location, StringRef method, const std::string& context)
            {
                if (method == "repaint")
                {
                    diag(location,
                         "calling 'repaint()' %0 causes an endless loop. Call "
                         "'repaint()' from a 'juce::Timer' callback if you "
                         "need to repaint repeatedly")
                        << context;
                }
                else if (method == "setBounds")
                {
                    diag(location,
                         "calling 'setBounds()' %0 changes the layout while "
                         "painting, triggering another repaint. Do this in "
                         "'resized()' instead")
                        << context;
                }
                else if (method == "setVisible")
                {
                    diag(location,
                         "calling 'setVisible()' %0 triggers another repaint. "
                         "Do this elsewhere")
                        << context;
                }
                else
                {
                    diag(location,
                         "calling '%0()' %1 changes the component hierarchy "
                         "while painting, and can crash if JUCE is iterating "
                         "over the children it changes. Do this elsewhere")
                        << method
                        << context;
                }
            }

            void followCalls(const Stmt& statement,
                             size_t frameIndex,
                             std::vector<Frame>& frames,
                             ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto& function = *frames.at(frameIndex).function;

                for (const auto& nodes : match(findAll(expr(anyOf(callExpr(), cxxConstructExpr()),
                                                            inFrame(function))
                                                           .bind("call")),
                                               statement,
                                               context))
                {
                    const auto& call = *nodes.getNodeAs<Expr>("call");

                    if (const auto* component = getConstructedComponent(call);
                        component != nullptr && !initialisesMember(cast<CXXConstructExpr>(call), function))
                    {
                        reportConstruction(call.getExprLoc(), *component, frameIndex, frames, context);
                    }

                    follow(call, frameIndex, frames, context);
                }
            }

            void follow(const Expr& call,
                        size_t frameIndex,
                        std::vector<Frame>& frames,
                        ASTContext& context)
            {
                const auto* definition = getDefinition(call);

                if (definition == nullptr)
                    return;

                if (!isOpaque(*definition, context.getSourceManager()))
                {
                    addFrame(*definition, call, frameIndex, frames);
                    return;
                }

                if (!definition->isTemplateInstantiation())
                    return;

                const auto& summary = summarise(*definition, context, 0);

                for (const auto* component : summary.components)
                    reportConstruction(call.getExprLoc(), *component, frameIndex, frames, context);

                for (const auto* callee : summary.callees)
                    addFrame(*callee, call, frameIndex, frames);
            }

            void addFrame(const FunctionDecl& function,
                          const Expr& callSite,
                          size_t caller,
                          std::vector<Frame>& frames)
            {
                if (isEntryPoint(function) || !checkedFunctions.insert(function.getCanonicalDecl()).second)
                    return;

                frames.push_back({
                    .function = &function,
                    .callSite = &callSite,
                    .caller = caller,
                });
            }

            // Calls into JUCE and the standard library aren't checked, but
            // their templates are looked through for the components they
            // create and the code they call back into, such as for
            // 'std::make_unique()'.
            const TemplateSummary& summarise(const FunctionDecl& function, ASTContext& context, int depth)
            {
                if (const auto existing = templateSummaries.find(&function); existing != templateSummaries.end())
                    return existing->second;

                auto& summary = templateSummaries[&function];

                if (depth >= maximumLookThroughDepth)
                    return summary;

                using namespace clang::ast_matchers;

                for (const auto* statement : utils::getFunctionStatements(function))
                {
                    for (const auto& nodes : match(findAll(expr(anyOf(callExpr(), cxxConstructExpr())).bind("call")),
                                                   *statement,
                                                   context))
                    {
                        const auto& call = *nodes.getNodeAs<Expr>("call");

                        if (const auto* component = getConstructedComponent(call))
                            summary.components.push_back(component);

                        const auto* definition = getDefinition(call);

                        if (definition == nullptr)
                            continue;

                        if (!isOpaque(*definition, context.getSourceManager()))
                        {
                            summary.callees.push_back(definition);
                            continue;
                        }

                        if (!definition->isTemplateInstantiation())
                            continue;

                        if (const auto& inner = summarise(*definition, context, depth + 1); &inner != &summary)
                        {
                            llvm::append_range(summary.components, inner.components);
                            llvm::append_range(summary.callees, inner.callees);
                        }
                    }
                }

                return summary;
            }

            [[nodiscard]] static const FunctionDecl* getDefinition(const Expr& call)
            {
                const auto* callee = getCallee(call);
                const FunctionDecl* definition = nullptr;

                if (callee == nullptr || callee->isImplicit() || callee->getBody(definition) == nullptr)
                    return nullptr;

                return definition;
            }

            [[nodiscard]] static const CXXRecordDecl* getConstructedComponent(const Expr& expression)
            {
                const auto* construction = dyn_cast<CXXConstructExpr>(&expression);

                if (construction == nullptr || construction->getConstructionKind() != CXXConstructionKind::Complete)
                    return nullptr;

                const auto* record = construction->getConstructor()->getParent();
                return utils::isSameOrDerivedFrom(record, componentNames) ? record : nullptr;
            }

            // Members are constructed as part of the object that owns them,
            // rather than created separately.
            [[nodiscard]] static bool initialisesMember(const CXXConstructExpr& construction, const FunctionDecl& function)
            {
                const auto* constructor = dyn_cast<CXXConstructorDecl>(&function);

                return constructor != nullptr
                    && llvm::any_of(constructor->inits(),
                                    [&construction](const CXXCtorInitializer* initialiser) {
                                        return initialiser->getInit() != nullptr
                                            && initialiser->getInit()->IgnoreImplicit() == &construction;
                                    });
            }

            [[nodiscard]] static std::string getDisplayName(const FunctionDecl& function)
            {
                if (const auto* method = dyn_cast<CXXMethodDecl>(&function);
                    method != nullptr && method->getParent()->isLambda())
                {
                    return "a lambda";
                }

                const auto name = isEntryPoint(function) ? function.getNameAsString() : utils::qualifiedName(function);
                return "'" + name + "()'";
            }

            [[nodiscard]] static std::string describeContext(size_t frameIndex, const std::vector<Frame>& frames)
            {
                const auto entryPoint = getDisplayName(*frames.front().function);

                if (frameIndex == 0)
                    return "inside " + entryPoint;

                return "inside " + getDisplayName(*frames.at(frameIndex).function) + ", which is called from " + entryPoint + ",";
            }

            void addCallChainNotes(size_t frameIndex, const std::vector<Frame>& frames)
            {
                std::vector<std::pair<const Frame*, const Frame*>> chain;

                for (const auto* frame = &frames.at(frameIndex); frame->caller.has_value();)
                {
                    const auto* caller = &frames.at(*frame->caller);
                    chain.emplace_back(frame, caller);
                    frame = caller;
                }

                for (const auto& [frame, caller] : llvm::reverse(chain))
                {
                    diag(frame->callSite->getBeginLoc(),
                         "%0 is called from %1 here",
                         DiagnosticIDs::Note)
                        << getDisplayName(*frame->function)
                        << getDisplayName(*caller->function);
                }
            }

            [[nodiscard]] static const FunctionDecl* getCallee(const Expr& call)
            {
                if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
                    return construction->getConstructor();

                return cast<CallExpr>(call).getDirectCallee();
            }

            llvm::DenseSet<const FunctionDecl*> checkedFunctions;
            std::map<const FunctionDecl*, TemplateSummary> templateSummaries;
            llvm::DenseSet<SourceLocation> reportedLocations;
        };
    } // namespace paint_side_effects

    using paint_side_effects::PaintSideEffectsCheck;
} // namespace clang::tidy::juce
