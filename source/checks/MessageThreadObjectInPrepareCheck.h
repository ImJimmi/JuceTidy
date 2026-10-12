#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/DenseSet.h>

namespace clang::tidy::juce
{
    namespace message_thread_object_in_prepare
    {
        class MessageThreadObjectInPrepareCheck : public JuceTidyCheck
        {
        public:
            MessageThreadObjectInPrepareCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawFunctions{ Options.get("Functions", defaultFunctions) }
                , functions{ utils::parseQualifiedNames(rawFunctions) }
                , rawMessageThreadTypes{ Options.get("MessageThreadTypes", defaultMessageThreadTypes) }
                , messageThreadTypes{ utils::parseQualifiedNames(rawMessageThreadTypes) }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "Functions", rawFunctions);
                Options.store(options, "MessageThreadTypes", rawMessageThreadTypes);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    functionDecl(isDefinition(),
                                 unless(isInstantiated()),
                                 unless(isExpansionInSystemHeader()),
                                 utils::isOrOverridesAnyOf(functions))
                        .bind("entryPoint"),
                    this);
            }

            void check(const ast_matchers::MatchFinder::MatchResult& result) override
            {
                const auto* entryPoint = result.Nodes.getNodeAs<FunctionDecl>("entryPoint");
                std::vector<Frame> frames{
                    {
                        .function = entryPoint,
                        .callSite = nullptr,
                    },
                };
                llvm::DenseSet<const FunctionDecl*> visited{ entryPoint->getCanonicalDecl() };

                for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
                {
                    for (const auto* statement : utils::getFunctionStatements(*frames.at(frameIndex).function))
                    {
                        reportCreations(*statement, frames, frameIndex, *result.Context);
                        reportDestructions(*statement, frames, frameIndex, *result.Context);
                        followCalls(*statement, frames, visited, *result.Context);
                    }
                }
            }

            void onStartOfTranslationUnit() override
            {
                reported.clear();
            }

        private:
            struct Frame
            {
                const FunctionDecl* function;
                const Expr* callSite;
            };

            static inline constexpr auto defaultFunctions =
                "::juce::AudioProcessor::prepareToPlay;"
                "::juce::AudioProcessor::releaseResources;"
                "::juce::AudioProcessor::numChannelsChanged;"
                "::juce::AudioSource::prepareToPlay;"
                "::juce::AudioSource::releaseResources";
            static inline constexpr auto defaultMessageThreadTypes =
                "::juce::Timer;"
                "::juce::MultiTimer;"
                "::juce::TimedCallback;"
                "::juce::AsyncUpdater;"
                "::juce::ChangeBroadcaster;"
                "::juce::Component;"
                "::juce::VBlankAttachment";

            void reportCreations(const Stmt& statement, const std::vector<Frame>& frames, size_t frameIndex, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto creation = expr(anyOf(cxxConstructExpr(unless(hasDeclaration(cxxConstructorDecl(anyOf(isCopyConstructor(),
                                                                                                                 isMoveConstructor())))))
                                                     .bind("construct"),
                                                 callExpr(callee(functionDecl(hasAnyName("::std::make_unique",
                                                                                         "::std::make_shared"),
                                                                              hasTemplateArgument(0, refersToType(qualType().bind("made"))))))))
                                          .bind("creation");

                for (const auto& nodes : match(findAll(creation), statement, context))
                {
                    const auto* node = nodes.getNodeAs<Expr>("creation");
                    const auto type = nodes.getNodeAs<CXXConstructExpr>("construct") != nullptr
                                        ? node->getType()
                                        : *nodes.getNodeAs<QualType>("made");

                    if (utils::isInMacroBody(node->getBeginLoc(), context.getSourceManager()) || !reported.insert(node).second)
                        continue;

                    if (const auto found = findMessageThreadType(type))
                    {
                        diag(node->getBeginLoc(),
                             "this creates a '%0', which should only happen on "
                             "the message thread, but '%1' may be called on "
                             "non-message threads")
                            << *found
                            << describe(frames.front());

                        addCallChainNotes(frames, frameIndex);
                    }
                }
            }

            void reportDestructions(const Stmt& statement, const std::vector<Frame>& frames, size_t frameIndex, ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto owner = expr(hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(classTemplateSpecializationDecl(hasAnyName("::std::unique_ptr",
                                                                                                                                                 "::std::shared_ptr",
                                                                                                                                                 "::std::optional"))
                                                                                                          .bind("owner"))))));
                const auto destruction = expr(anyOf(cxxOperatorCallExpr(hasOverloadedOperatorName("="),
                                                                        hasArgument(0, owner)),
                                                    cxxMemberCallExpr(callee(cxxMethodDecl(hasName("reset"))),
                                                                      on(owner))))
                                             .bind("destruction");

                for (const auto& nodes : match(findAll(destruction), statement, context))
                {
                    const auto* node = nodes.getNodeAs<Expr>("destruction");
                    const auto* ownerType = nodes.getNodeAs<ClassTemplateSpecializationDecl>("owner");

                    if (utils::isInMacroBody(node->getBeginLoc(), context.getSourceManager())
                        || ownerType->getTemplateArgs().size() == 0
                        || !reported.insert(node).second)
                    {
                        continue;
                    }

                    if (const auto found = findMessageThreadType(ownerType->getTemplateArgs().get(0).getAsType()))
                    {
                        diag(node->getBeginLoc(),
                             "this may destroy a '%0', which should only "
                             "happen on the message thread, but '%1' may be "
                             "called on non-message threads")
                            << *found
                            << describe(frames.front());

                        addCallChainNotes(frames, frameIndex);
                    }
                }
            }

            void followCalls(const Stmt& statement, std::vector<Frame>& frames, llvm::DenseSet<const FunctionDecl*>& visited, ASTContext& context) const
            {
                using namespace clang::ast_matchers;

                for (const auto& nodes : match(findAll(callExpr().bind("call")), statement, context))
                {
                    const auto* call = nodes.getNodeAs<CallExpr>("call");
                    const auto* callee = call->getDirectCallee();
                    const FunctionDecl* definition = nullptr;

                    if (callee == nullptr
                        || context.getSourceManager().isInSystemHeader(callee->getLocation())
                        || utils::isInJuceNamespace(*callee)
                        || callee->getBody(definition) == nullptr
                        || definition == nullptr)
                    {
                        continue;
                    }

                    if (const auto* method = dyn_cast<CXXMethodDecl>(callee); method != nullptr && method->isVirtual())
                        continue;

                    if (visited.insert(definition->getCanonicalDecl()).second)
                    {
                        frames.push_back({
                            .function = definition,
                            .callSite = call,
                        });
                    }
                }
            }

            void addCallChainNotes(const std::vector<Frame>& frames, size_t frameIndex)
            {
                if (frameIndex == 0)
                    return;

                diag(frames.at(frameIndex).callSite->getBeginLoc(),
                     "%0 is called from %1 here",
                     DiagnosticIDs::Note)
                    << frames.at(frameIndex).function
                    << describe(frames.front());
            }

            [[nodiscard]] static std::string describe(const Frame& frame)
            {
                return "'" + frame.function->getNameAsString() + "()'";
            }

            [[nodiscard]] std::optional<std::string> findMessageThreadType(QualType type) const
            {
                llvm::DenseSet<const CXXRecordDecl*> visitedRecords;
                return findMessageThreadType(type, visitedRecords);
            }

            [[nodiscard]] std::optional<std::string> findMessageThreadType(QualType type, llvm::DenseSet<const CXXRecordDecl*>& visitedRecords) const
            {
                const auto* record = type.isNull() ? nullptr : type->getAsCXXRecordDecl();

                if (record == nullptr || !record->hasDefinition() || !visitedRecords.insert(record->getCanonicalDecl()).second)
                    return std::nullopt;

                record = record->getDefinition();

                if (auto name = utils::qualifiedName(*record); llvm::is_contained(messageThreadTypes, name))
                    return name;

                if (const auto* specialisation = dyn_cast<ClassTemplateSpecializationDecl>(record))
                {
                    const auto name = utils::qualifiedName(*specialisation);

                    if (name == "std::unique_ptr" || name == "std::optional" || name == "std::shared_ptr")
                        return specialisation->getTemplateArgs().size() == 0
                                 ? std::nullopt
                                 : findMessageThreadType(specialisation->getTemplateArgs().get(0).getAsType(), visitedRecords);
                }

                for (const auto& base : record->bases())
                    if (const auto found = findMessageThreadType(base.getType(), visitedRecords))
                        return found;

                for (const auto* field : record->fields())
                    if (const auto found = findMessageThreadType(field->getType()->isArrayType()
                                                                     ? QualType{ field->getType()->getPointeeOrArrayElementType(), 0 }
                                                                     : field->getType(),
                                                                 visitedRecords))
                        return found;

                return std::nullopt;
            }

            const StringRef rawFunctions;
            const std::vector<std::string> functions;
            const StringRef rawMessageThreadTypes;
            const std::vector<std::string> messageThreadTypes;

            llvm::DenseSet<const Stmt*> reported;
        };
    } // namespace message_thread_object_in_prepare

    using message_thread_object_in_prepare::MessageThreadObjectInPrepareCheck;
} // namespace clang::tidy::juce
