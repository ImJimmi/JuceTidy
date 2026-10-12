#pragma once

#include "utils/JuceTidyUtils.h"

#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallPtrSet.h>

namespace clang::tidy::juce
{
    namespace realtime_safety
    {
        [[nodiscard]] inline bool hasNonBlockingEffect(const FunctionDecl& function)
        {
            return llvm::any_of(function.getFunctionEffects().effects(),
                                [](const FunctionEffect& effect) {
                                    return effect.kind() == FunctionEffect::Kind::NonBlocking;
                                });
        }

        AST_MATCHER(FunctionDecl, isMarkedNonBlocking)
        {
            return hasNonBlockingEffect(Node);
        }

        AST_MATCHER(ClassTemplateSpecializationDecl, isNonLockFreeAtomic)
        {
            if (utils::qualifiedName(Node) != "std::atomic" || Node.getTemplateArgs().size() != 1)
                return false;

            const auto valueType = Node.getTemplateArgs().get(0).getAsType();

            if (valueType.isNull() || valueType->isDependentType() || valueType->isIncompleteType())
                return false;

            const auto size = Finder->getASTContext().getTypeSizeInChars(valueType).getQuantity();
            return size != 1 && size != 2 && size != 4 && size != 8;
        }

        class RealtimeSafetyCheck : public JuceTidyCheck
        {
        public:
            RealtimeSafetyCheck(StringRef name, ClangTidyContext* context)
                : JuceTidyCheck{ name, context }
                , rawAudioCallbacks{ Options.get("AudioCallbacks", defaultAudioCallbacks) }
                , audioCallbacks{ utils::parseQualifiedNames(rawAudioCallbacks) }
                , rawParameterCallbacks{ Options.get("ParameterCallbacks", defaultParameterCallbacks) }
                , parameterCallbacks{ utils::parseQualifiedNames(rawParameterCallbacks) }
                , rawUnsafeFunctions{ Options.get("UnsafeFunctions", "") }
                , unsafeFunctions{ utils::parseQualifiedNames(rawUnsafeFunctions) }
                , rawUnsafeMacros{ Options.get("UnsafeMacros", "DBG") }
                , unsafeMacros{ utils::parseQualifiedNames(rawUnsafeMacros) }
                , rawOpaqueNamespaces{ Options.get("OpaqueNamespaces", defaultOpaqueNamespaces) }
                , opaqueNamespaces{ utils::parseQualifiedNames(rawOpaqueNamespaces) }
                , checkParameterLookups{ Options.get("CheckParameterLookups", true) }
                , reportUncheckedCalls{ Options.get("ReportUncheckedCalls", false) }
                , rules{ makeRules() }
            {
            }

            void storeOptions(ClangTidyOptions::OptionMap& options) override
            {
                Options.store(options, "AudioCallbacks", rawAudioCallbacks);
                Options.store(options, "ParameterCallbacks", rawParameterCallbacks);
                Options.store(options, "UnsafeFunctions", rawUnsafeFunctions);
                Options.store(options, "UnsafeMacros", rawUnsafeMacros);
                Options.store(options, "OpaqueNamespaces", rawOpaqueNamespaces);
                Options.store(options, "CheckParameterLookups", checkParameterLookups);
                Options.store(options, "ReportUncheckedCalls", reportUncheckedCalls);
            }

            void registerMatchers(ast_matchers::MatchFinder* finder) override
            {
                using namespace clang::ast_matchers;

                finder->addMatcher(
                    functionDecl(isDefinition(),
                                 unless(isInstantiated()),
                                 unless(isExpansionInSystemHeader()),
                                 anyOf(utils::isOrOverridesAnyOf(audioCallbacks),
                                       utils::isOrOverridesAnyOf(parameterCallbacks),
                                       isMarkedNonBlocking()))
                        .bind("entryPoint"),
                    this);
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

            void registerPPCallbacks(const SourceManager& sourceManager,
                                     Preprocessor* preprocessor,
                                     Preprocessor*) override
            {
                unsafeMacroExpansions.clear();
                reportedMacroExpansions.clear();
                checkedFunctions.clear();
                reportedNodes.clear();

                preprocessor->addPPCallbacks(std::make_unique<utils::MacroExpansionListener>(
                    unsafeMacros,
                    [this, &sourceManager](const Token& name, SourceRange, const MacroArgs*) {
                        if (!name.getLocation().isMacroID()
                            && !sourceManager.isInSystemHeader(name.getLocation()))
                        {
                            unsafeMacroExpansions.push_back(name.getLocation());
                        }
                    }));
            }

        private:
            struct Rule
            {
                ast_matchers::internal::Matcher<Stmt> matcher;
                StringRef reason;
            };

            struct Frame
            {
                const FunctionDecl* function;
                const Expr* callSite;
                std::optional<size_t> caller;
            };

            static inline constexpr auto defaultParameterCallbacks =
                "::juce::AudioProcessorValueTreeState::Listener::parameterChanged;"
                "::juce::AudioProcessorParameter::Listener::parameterValueChanged;"
                "::juce::AudioProcessorParameter::Listener::parameterGestureChanged;"
                "::juce::AudioProcessorListener::audioProcessorParameterChanged;"
                "::juce::AudioParameterFloat::valueChanged;"
                "::juce::AudioParameterInt::valueChanged;"
                "::juce::AudioParameterBool::valueChanged;"
                "::juce::AudioParameterChoice::valueChanged";
            static inline constexpr auto defaultAudioCallbacks =
                "::juce::AudioProcessor::processBlock;"
                "::juce::AudioProcessor::processBlockBypassed;"
                "::juce::AudioSource::getNextAudioBlock;"
                "::juce::AudioIODeviceCallback::audioDeviceIOCallbackWithContext;"
                "::juce::SynthesiserVoice::renderNextBlock;"
                "::juce::MPESynthesiserVoice::renderNextBlock;"
                "::juce::dsp::ProcessorBase::process";
            static inline constexpr auto defaultOpaqueNamespaces =
                "::juce;"
                "::std";

            [[nodiscard]] std::vector<Rule> makeRules() const
            {
                using namespace clang::ast_matchers;

                const std::vector<StringRef> standardContainers{
                    "::std::vector",
                    "::std::basic_string",
                    "::std::deque",
                    "::std::list",
                    "::std::forward_list",
                    "::std::map",
                    "::std::multimap",
                    "::std::set",
                    "::std::multiset",
                    "::std::unordered_map",
                    "::std::unordered_multimap",
                    "::std::unordered_set",
                    "::std::unordered_multiset",
                };
                const std::vector<StringRef> growingMethods{
                    "push_back",
                    "emplace_back",
                    "push_front",
                    "emplace_front",
                    "insert",
                    "emplace",
                    "emplace_hint",
                    "resize",
                    "reserve",
                    "assign",
                    "append",
                    "shrink_to_fit",
                    "try_emplace",
                    "insert_or_assign",
                };
                const auto lockType = cxxRecordDecl(anyOf(hasAnyName("::std::lock_guard",
                                                                     "::std::unique_lock",
                                                                     "::std::scoped_lock",
                                                                     "::std::shared_lock",
                                                                     "::juce::ScopedReadLock",
                                                                     "::juce::ScopedWriteLock",
                                                                     "::juce::MessageManagerLock"),
                                                          classTemplateSpecializationDecl(hasName("::juce::GenericScopedLock"),
                                                                                          unless(hasTemplateArgument(0,
                                                                                                                     refersToType(hasDeclaration(cxxRecordDecl(hasName("::juce::SpinLock")))))))));

                std::vector<Rule> rules{
                    {
                        .matcher = cxxNewExpr(),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxDeleteExpr(),
                        .reason = "frees memory",
                    },
                    {
                        .matcher = cxxThrowExpr(),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = callTo({
                            "::malloc",
                            "::calloc",
                            "::realloc",
                            "::free",
                            "::strdup",
                            "::std::make_unique",
                            "::std::make_shared",
                            "::std::allocate_shared",
                        }),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = methodOf(standardContainers, growingMethods),
                        .reason = "may allocate memory",
                    },
                    {
                        .matcher = cxxOperatorCallExpr(hasOverloadedOperatorName("[]"),
                                                       callee(cxxMethodDecl(ofClass(hasAnyName("::std::map",
                                                                                               "::std::unordered_map"))))),
                        .reason = "may allocate memory",
                    },
                    {
                        .matcher = methodOf({
                                                "::juce::Array",
                                                "::juce::OwnedArray",
                                                "::juce::ReferenceCountedArray",
                                                "::juce::StringArray",
                                                "::juce::SortedSet",
                                            },
                                            {
                                                "add",
                                                "addArray",
                                                "addIfNotAlreadyThere",
                                                "addSorted",
                                                "addUsingDefaultSort",
                                                "insert",
                                                "insertMultiple",
                                                "insertArray",
                                                "resize",
                                                "ensureStorageAllocated",
                                            }),
                        .reason = "may allocate memory",
                    },
                    {
                        .matcher = methodOf({ "::juce::MemoryBlock" }, {
                                                                           "setSize",
                                                                           "ensureSize",
                                                                           "append",
                                                                           "insert",
                                                                           "replaceAll",
                                                                       }),
                        .reason = "may allocate memory",
                    },
                    {
                        .matcher = methodOf({ "::juce::HeapBlock" }, {
                                                                         "malloc",
                                                                         "calloc",
                                                                         "realloc",
                                                                         "allocate",
                                                                     }),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("setSize"), ofClass(hasName("::juce::AudioBuffer")))), withoutAvoidReallocating(4)),
                        .reason = "may allocate memory unless 'avoidReallocating' is true",
                    },
                    {
                        .matcher = cxxMemberCallExpr(callee(cxxMethodDecl(hasName("makeCopyOf"), ofClass(hasName("::juce::AudioBuffer")))), withoutAvoidReallocating(1)),
                        .reason = "may allocate memory unless 'avoidReallocating' is true",
                    },
                    {
                        .matcher = stringConstruction(),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = methodOf({ "::juce::String" }, {
                                                                      "trim",
                                                                      "trimStart",
                                                                      "trimEnd",
                                                                      "trimCharactersAtStart",
                                                                      "trimCharactersAtEnd",
                                                                      "toUpperCase",
                                                                      "toLowerCase",
                                                                      "replace",
                                                                      "replaceCharacter",
                                                                      "replaceCharacters",
                                                                      "replaceSection",
                                                                      "replaceFirstOccurrenceOf",
                                                                      "substring",
                                                                      "dropLastCharacters",
                                                                      "getLastCharacters",
                                                                      "fromFirstOccurrenceOf",
                                                                      "fromLastOccurrenceOf",
                                                                      "upToFirstOccurrenceOf",
                                                                      "upToLastOccurrenceOf",
                                                                      "quoted",
                                                                      "unquoted",
                                                                      "paddedLeft",
                                                                      "paddedRight",
                                                                      "removeCharacters",
                                                                      "retainCharacters",
                                                                      "initialSectionContainingOnly",
                                                                      "initialSectionNotContaining",
                                                                      "toStdString",
                                                                  }),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = callTo({
                            "::juce::String::formatted",
                            "::juce::String::repeatedString",
                            "::juce::String::toHexString",
                        }),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxOperatorCallExpr(hasAnyOverloadedOperatorName("+", "+=", "<<"), hasAnyArgument(hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(namedDecl(stringType()))))))),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxConstructExpr(hasType(hasUnqualifiedDesugaredType(recordType(hasDeclaration(lockType))))),
                        .reason = "may block",
                    },
                    {
                        .matcher = methodOf({
                                                "::std::mutex",
                                                "::std::recursive_mutex",
                                                "::std::timed_mutex",
                                                "::std::recursive_timed_mutex",
                                                "::std::shared_mutex",
                                            },
                                            {
                                                "lock",
                                                "lock_shared",
                                            }),
                        .reason = "may block",
                    },
                    {
                        .matcher = methodOf({
                                                "::juce::CriticalSection",
                                                "::juce::ReadWriteLock",
                                            },
                                            {
                                                "enter",
                                                "enterRead",
                                                "enterWrite",
                                            }),
                        .reason = "may block",
                    },
                    {
                        .matcher = methodOf({
                                                "::juce::WaitableEvent",
                                                "::std::condition_variable",
                                                "::std::condition_variable_any",
                                            },
                                            {
                                                "wait",
                                                "wait_for",
                                                "wait_until",
                                            }),
                        .reason = "may block",
                    },
                    {
                        .matcher = methodOf({ "::juce::Thread" }, {
                                                                      "startThread",
                                                                      "stopThread",
                                                                      "waitForThreadToExit",
                                                                  }),
                        .reason = "may block",
                    },
                    {
                        .matcher = callTo({
                            "::juce::Thread::sleep",
                            "::std::this_thread::sleep_for",
                            "::std::this_thread::sleep_until",
                        }),
                        .reason = "may block",
                    },
                    {
                        .matcher = callTo({
                            "::juce::MessageManager::callAsync",
                            "::juce::MessageManager::callFunctionOnMessageThread",
                            "::juce::AsyncUpdater::triggerAsyncUpdate",
                            "::juce::ChangeBroadcaster::sendChangeMessage",
                            "::juce::ChangeBroadcaster::sendSynchronousChangeMessage",
                            "::juce::ActionBroadcaster::sendActionMessage",
                        }),
                        .reason = "posts a message to the message thread, which may block or allocate",
                    },
                    {
                        .matcher = anyMethodOf({ "::juce::Component" }),
                        .reason = "must only be called on the message thread",
                    },
                    {
                        .matcher = anyMethodOf({
                            "::juce::ValueTree",
                            "::juce::CachedValue",
                        }),
                        .reason = "isn't thread-safe",
                    },
                    {
                        .matcher = callTo({
                            "::printf",
                            "::fprintf",
                            "::puts",
                            "::fputs",
                            "::putchar",
                            "::std::print",
                            "::std::println",
                            "::juce::Logger::writeToLog",
                            "::juce::Logger::outputDebugString",
                        }),
                        .reason = "performs I/O",
                    },
                    {
                        .matcher = declRefExpr(to(varDecl(hasAnyName("::std::cout", "::std::cerr", "::std::clog", "::std::wcout", "::std::wcerr")))),
                        .reason = "performs I/O",
                    },
                    {
                        .matcher = anyMethodOf({
                            "::juce::File",
                            "::juce::FileInputStream",
                            "::juce::FileOutputStream",
                        }),
                        .reason = "performs I/O",
                    },
                    {
                        .matcher = callExpr(callee(cxxMethodDecl(matchesName("::make[A-Za-z]*$"), ofClass(hasName("::juce::dsp::IIR::Coefficients"))))),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxConstructExpr(hasDeclaration(cxxConstructorDecl(isCopyConstructor(), ofClass(hasName("::juce::AudioBuffer"))))),
                        .reason = "allocates memory",
                    },
                    {
                        .matcher = cxxOperatorCallExpr(hasOverloadedOperatorName("="), callee(cxxMethodDecl(isCopyAssignmentOperator(), ofClass(hasName("::juce::AudioBuffer"))))),
                        .reason = "may allocate memory",
                    },
                    {
                        .matcher = cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasName("::juce::Identifier")), unless(isCopyConstructor()), unless(isMoveConstructor())))),
                        .reason = "locks JUCE's global string pool",
                    },
                    {
                        .matcher = expr(anyOf(methodOf({ "::std::weak_ptr" }, { "lock" }), cxxConstructExpr(hasDeclaration(cxxConstructorDecl(isCopyConstructor(), ofClass(hasName("::std::shared_ptr"))))))),
                        .reason = "takes shared ownership, so the object may be destroyed on the audio thread",
                    },
                    {
                        .matcher = expr(anyOf(cxxOperatorCallExpr(hasOverloadedOperatorName("="), callee(cxxMethodDecl(ofClass(hasName("::std::shared_ptr"))))), methodOf({ "::std::shared_ptr" }, { "reset" }))),
                        .reason = "may destroy the shared object on the audio thread",
                    },
                    {
                        .matcher = expr(anyOf(cxxMemberCallExpr(on(nonLockFreeAtomic())), cxxOperatorCallExpr(hasArgument(0, nonLockFreeAtomic())))),
                        .reason = "isn't lock-free for this type, so it may lock",
                    },
                };

                if (checkParameterLookups)
                {
                    rules.push_back({
                        .matcher = methodOf({ "::juce::AudioProcessorValueTreeState" },
                                            {
                                                "getParameter",
                                                "getRawParameterValue",
                                                "getParameterAsValue",
                                                "getParameterRange",
                                            }),
                        .reason = "looks up the parameter by its ID on every call",
                    });
                }

                if (!unsafeFunctions.empty())
                {
                    rules.push_back({
                        .matcher = callExpr(callee(functionDecl(utils::isOrOverridesAnyOf(unsafeFunctions)))),
                        .reason = "is listed in 'UnsafeFunctions'",
                    });
                }

                return rules;
            }

            [[nodiscard]] bool isAudioCallback(const FunctionDecl& function) const
            {
                return utils::isOrOverridesAnyOf(function, audioCallbacks);
            }

            [[nodiscard]] bool isParameterCallback(const FunctionDecl& function) const
            {
                return utils::isOrOverridesAnyOf(function, parameterCallbacks);
            }

            [[nodiscard]] bool isEntryPoint(const FunctionDecl& function) const
            {
                return isAudioCallback(function)
                    || isParameterCallback(function)
                    || hasNonBlockingEffect(function);
            }

            [[nodiscard]] bool isOpaque(const FunctionDecl& function, const SourceManager& sourceManager) const
            {
                if (sourceManager.isInSystemHeader(sourceManager.getExpansionLoc(function.getLocation())))
                    return true;

                const auto name = utils::qualifiedName(function);

                return llvm::any_of(opaqueNamespaces,
                                    [&name](const std::string& opaqueNamespace) {
                                        return StringRef(name).starts_with(opaqueNamespace + "::");
                                    });
            }

            [[nodiscard]] bool isInUnsafeMacro(SourceLocation location, const SourceManager& sourceManager) const
            {
                return location.isMacroID()
                    && llvm::is_contained(unsafeMacroExpansions, sourceManager.getExpansionLoc(location));
            }

            [[nodiscard]] std::string getDisplayName(const FunctionDecl& function) const
            {
                const auto isCallback = isAudioCallback(function) || isParameterCallback(function);
                return "'" + (isCallback ? function.getNameAsString() : utils::qualifiedName(function)) + "'";
            }

            [[nodiscard]] std::string describeContext(size_t frameIndex, const std::vector<Frame>& frames) const
            {
                const auto& entryPoint = *frames.front().function;
                const auto entryPointName = getDisplayName(entryPoint);
                const auto prefix = frameIndex == 0
                                      ? std::string{}
                                      : getDisplayName(*frames.at(frameIndex).function) + ", which is called from ";

                if (isAudioCallback(entryPoint))
                    return prefix + "the real-time audio callback " + entryPointName;

                if (isParameterCallback(entryPoint))
                    return prefix + entryPointName + ", a parameter callback that can run on the audio thread";

                return prefix + "the nonblocking function " + entryPointName;
            }

            void checkFrame(size_t frameIndex, std::vector<Frame>& frames, ASTContext& context)
            {
                for (const auto* statement : utils::getFunctionStatements(*frames.at(frameIndex).function))
                {
                    reportUnsafeOperations(*statement, frameIndex, frames, context);
                    reportUnsafeMacros(*statement, frameIndex, frames, context);
                    followCalls(*statement, frameIndex, frames, context);
                }
            }

            void reportUnsafeOperations(const Stmt& statement,
                                        size_t frameIndex,
                                        const std::vector<Frame>& frames,
                                        ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto& sourceManager = context.getSourceManager();

                for (const auto& [matcher, reason] : rules)
                {
                    for (const auto& nodes : match(findAll(stmt(matcher).bind("unsafe")),
                                                   statement,
                                                   context))
                    {
                        const auto* node = nodes.getNodeAs<Stmt>("unsafe");
                        const auto location = node->getBeginLoc();

                        if (utils::isInMacroBody(location, sourceManager)
                            || isInUnsafeMacro(location, sourceManager)
                            || !reportedNodes.insert(node).second)
                        {
                            continue;
                        }

                        diag(location,
                             "%0 %1. Don't use it in %2")
                            << describe(*node)
                            << reason
                            << describeContext(frameIndex, frames);

                        addCallChainNotes(frameIndex, frames);
                    }
                }
            }

            void reportUnsafeMacros(const Stmt& statement,
                                    size_t frameIndex,
                                    const std::vector<Frame>& frames,
                                    ASTContext& context)
            {
                const auto& sourceManager = context.getSourceManager();

                for (const auto location : unsafeMacroExpansions)
                {
                    if (!sourceManager.isPointWithin(location, statement.getBeginLoc(), statement.getEndLoc())
                        || !reportedMacroExpansions.insert(location).second)
                    {
                        continue;
                    }

                    diag(location,
                         "'%0' performs I/O and allocates memory. Don't use it "
                         "in %1")
                        << Lexer::getSourceText(CharSourceRange::getTokenRange(location), sourceManager, context.getLangOpts())
                        << describeContext(frameIndex, frames);

                    addCallChainNotes(frameIndex, frames);
                }
            }

            void followCalls(const Stmt& statement,
                             size_t frameIndex,
                             std::vector<Frame>& frames,
                             ASTContext& context)
            {
                using namespace clang::ast_matchers;

                const auto& sourceManager = context.getSourceManager();

                for (const auto& nodes : match(findAll(expr(anyOf(callExpr(),
                                                                  cxxConstructExpr()))
                                                           .bind("call")),
                                               statement,
                                               context))
                {
                    const auto* call = nodes.getNodeAs<Expr>("call");
                    const auto* callee = getCallee(*call);

                    if (callee == nullptr
                        || callee->isImplicit()
                        || reportedNodes.contains(call)
                        || isInUnsafeMacro(call->getBeginLoc(), sourceManager))
                    {
                        continue;
                    }

                    if (isEntryPoint(*callee) || isOpaque(*callee, sourceManager))
                        continue;

                    if (const FunctionDecl* definition = nullptr;
                        callee->getBody(definition) != nullptr && definition != nullptr)
                    {
                        if (checkedFunctions.insert(definition->getCanonicalDecl()).second)
                        {
                            frames.push_back({
                                .function = definition,
                                .callSite = call,
                                .caller = frameIndex,
                            });
                        }

                        continue;
                    }

                    if (!reportUncheckedCalls
                        || callee->isDefaulted()
                        || callee->isDeleted()
                        || callee->getBuiltinID() != 0
                        || utils::isInMacroBody(call->getBeginLoc(), sourceManager)
                        || !reportedNodes.insert(call).second)
                    {
                        continue;
                    }

                    diag(call->getBeginLoc(),
                         "%0 isn't defined in this file, so it can't be "
                         "checked as part of %1; mark it "
                         "[[clang::nonblocking]] so it's checked where it's "
                         "defined")
                        << getDisplayName(*callee)
                        << describeContext(frameIndex, frames);

                    addCallChainNotes(frameIndex, frames);
                }
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

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> callTo(ArrayRef<StringRef> names)
            {
                using namespace clang::ast_matchers;

                return callExpr(callee(functionDecl(hasAnyName(names))));
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> methodOf(ArrayRef<StringRef> classes,
                                                                                        ArrayRef<StringRef> methods)
            {
                using namespace clang::ast_matchers;

                return cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName(methods),
                                                              ofClass(isSameOrDerivedFrom(hasAnyName(classes))))));
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> anyMethodOf(ArrayRef<StringRef> classes)
            {
                using namespace clang::ast_matchers;

                return cxxMemberCallExpr(callee(cxxMethodDecl(ofClass(isSameOrDerivedFrom(hasAnyName(classes))))));
            }

            [[nodiscard]] static ast_matchers::internal::Matcher<Expr> nonLockFreeAtomic()
            {
                using namespace clang::ast_matchers;

                return expr(hasType(hasCanonicalType(hasDeclaration(classTemplateSpecializationDecl(isNonLockFreeAtomic())))));
            }

            [[nodiscard]] static ast_matchers::internal::Matcher<NamedDecl> stringType()
            {
                using namespace clang::ast_matchers;

                return hasAnyName("::juce::String",
                                  "::std::basic_string");
            }

            [[nodiscard]] static ast_matchers::internal::BindableMatcher<Stmt> stringConstruction()
            {
                using namespace clang::ast_matchers;

                return cxxConstructExpr(hasDeclaration(cxxConstructorDecl(unless(isMoveConstructor()),
                                                                          unless(isDefaultConstructor()),
                                                                          anyOf(ofClass(hasName("::std::basic_string")),
                                                                                allOf(ofClass(hasName("::juce::String")),
                                                                                      unless(isCopyConstructor()))))));
            }

            [[nodiscard]] static ast_matchers::internal::Matcher<CXXMemberCallExpr> withoutAvoidReallocating(unsigned int argumentIndex)
            {
                using namespace clang::ast_matchers;

                return unless(hasArgument(argumentIndex, ignoringParenImpCasts(cxxBoolLiteral(equals(true)))));
            }

            [[nodiscard]] static std::string describe(const Stmt& node)
            {
                if (isa<CXXNewExpr>(node))
                    return "'new'";

                if (isa<CXXDeleteExpr>(node))
                    return "'delete'";

                if (isa<CXXThrowExpr>(node))
                    return "throwing an exception";

                if (const auto* reference = dyn_cast<DeclRefExpr>(&node))
                    return "using '" + utils::qualifiedName(*reference->getDecl()) + "'";

                if (const auto* call = dyn_cast<CXXMemberCallExpr>(&node);
                    call != nullptr && call->getMethodDecl() != nullptr && call->getMethodDecl()->getParent()->getName().starts_with("__"))
                {
                    auto policy = PrintingPolicy{ LangOptions{} };
                    policy.SuppressTagKeyword = true;

                    const auto objectType = call->getImplicitObjectArgument()->IgnoreParenBaseCasts()->getType().getUnqualifiedType();
                    return "calling '" + objectType.getAsString(policy) + "::" + call->getMethodDecl()->getNameAsString() + "'";
                }

                if (const auto* call = dyn_cast<CallExpr>(&node);
                    call != nullptr && call->getDirectCallee() != nullptr)
                {
                    const auto* callee = call->getDirectCallee();
                    const auto* method = dyn_cast<CXXMethodDecl>(callee);
                    const auto name = method != nullptr && !callee->getDeclName().isIdentifier()
                                        ? utils::qualifiedName(*method->getParent()) + "::" + callee->getNameAsString()
                                        : utils::qualifiedName(*callee);
                    return "calling '" + name + "'";
                }

                if (const auto* construct = dyn_cast<CXXConstructExpr>(&node))
                    return "constructing '" + utils::qualifiedName(*construct->getConstructor()->getParent()) + "'";

                return "this";
            }

            [[nodiscard]] static const FunctionDecl* getCallee(const Expr& call)
            {
                if (const auto* construction = dyn_cast<CXXConstructExpr>(&call))
                    return construction->getConstructor();

                return cast<CallExpr>(call).getDirectCallee();
            }

            const StringRef rawAudioCallbacks;
            const std::vector<std::string> audioCallbacks;
            const StringRef rawParameterCallbacks;
            const std::vector<std::string> parameterCallbacks;
            const StringRef rawUnsafeFunctions;
            const std::vector<std::string> unsafeFunctions;
            const StringRef rawUnsafeMacros;
            const std::vector<std::string> unsafeMacros;
            const StringRef rawOpaqueNamespaces;
            const std::vector<std::string> opaqueNamespaces;
            const bool checkParameterLookups;
            const bool reportUncheckedCalls;
            const std::vector<Rule> rules;

            std::vector<SourceLocation> unsafeMacroExpansions;
            llvm::DenseSet<SourceLocation> reportedMacroExpansions;
            llvm::DenseSet<const FunctionDecl*> checkedFunctions;
            llvm::DenseSet<const Stmt*> reportedNodes;
        };
    } // namespace realtime_safety

    using realtime_safety::RealtimeSafetyCheck;
} // namespace clang::tidy::juce
