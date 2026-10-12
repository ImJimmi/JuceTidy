#include "checks/ApvtsStateAccessCheck.h"
#include "checks/AsyncThisCaptureCheck.h"
#include "checks/AtomicInSampleLoopCheck.h"
#include "checks/AttachmentOrderCheck.h"
#include "checks/BlockSizeAssumptionCheck.h"
#include "checks/BufferPointerInSampleLoopCheck.h"
#include "checks/BufferedParentCheck.h"
#include "checks/CallbackDanglingCaptureCheck.h"
#include "checks/DuplicateWebviewNameCheck.h"
#include "checks/EmptyStateInformationCheck.h"
#include "checks/ExpensivePaintCheck.h"
#include "checks/ExplicitBaseAccessCheck.h"
#include "checks/ExplicitNullptrComparisonCheck.h"
#include "checks/ExplicitUnsignedIntCheck.h"
#include "checks/FloatEqualityCheck.h"
#include "checks/FloatMaxGeometryCheck.h"
#include "checks/GraphicsSaveStateCheck.h"
#include "checks/HardcodedResourceProviderRootCheck.h"
#include "checks/IdentifierFromLiteralCheck.h"
#include "checks/IgnoreUnusedCheck.h"
#include "checks/LeakDetectorCheck.h"
#include "checks/ListenerOnCopyCheck.h"
#include "checks/LocalFileChooserCheck.h"
#include "checks/LookAndFeelResetCheck.h"
#include "checks/MacroConditionStyleCheck.h"
#include "checks/ManualDeletionCheck.h"
#include "checks/ManualResizedCallCheck.h"
#include "checks/MemberAccessOrderCheck.h"
#include "checks/MessageManagerLockUncheckedCheck.h"
#include "checks/MessageThreadObjectInPrepareCheck.h"
#include "checks/MissingAccessibilityHandlerCheck.h"
#include "checks/MissingRemoveListenerCheck.h"
#include "checks/NativeFunctionCompletionCheck.h"
#include "checks/NonAsciiStringLiteralCheck.h"
#include "checks/NormalisedParameterValueCheck.h"
#include "checks/NullableDereferenceCheck.h"
#include "checks/NumericLiteralsCheck.h"
#include "checks/OpaqueWithoutFillCheck.h"
#include "checks/OpenGlDetachCheck.h"
#include "checks/PaintSideEffectsCheck.h"
#include "checks/ParameterIdLimitsCheck.h"
#include "checks/ParameterVersionHintCheck.h"
#include "checks/PassSmallObjectsByValueCheck.h"
#include "checks/PlatformMacrosCheck.h"
#include "checks/PluginGlobalStateCheck.h"
#include "checks/PreferConstexprCheck.h"
#include "checks/PreferJassertCheck.h"
#include "checks/PreferLoggerCheck.h"
#include "checks/PreferPreIncrementCheck.h"
#include "checks/PreferStartTimerHzCheck.h"
#include "checks/PreferStdContainersCheck.h"
#include "checks/PreferTimedCallbackCheck.h"
#include "checks/PrivateCallbackInheritanceCheck.h"
#include "checks/ProcessorEditorCouplingCheck.h"
#include "checks/ProcessorStateFromEditorCheck.h"
#include "checks/ProgramCountCheck.h"
#include "checks/PropertyNameCaseCheck.h"
#include "checks/RealtimeSafetyCheck.h"
#include "checks/RelativeFilePathCheck.h"
#include "checks/RepaintTimerCheck.h"
#include "checks/ScopedNoDenormalsCheck.h"
#include "checks/SeparatorCommentsCheck.h"
#include "checks/StateTypeCheck.h"
#include "checks/StdMathFunctionsCheck.h"
#include "checks/StopInDestructorCheck.h"
#include "checks/TemplateParameterNameCheck.h"
#include "checks/ThreadShouldExitCheck.h"
#include "checks/UnaddedChildComponentCheck.h"
#include "checks/UnboundCachedValueCheck.h"
#include "checks/UncheckedChildTreeCheck.h"
#include "checks/UncheckedSampleRateCheck.h"
#include "checks/UndoManagerConsistencyCheck.h"
#include "checks/UnitTestAssertionsCheck.h"
#include "checks/UnknownParameterIdCheck.h"
#include "checks/UnownedChildComponentCheck.h"
#include "checks/UnpreparedDspCheck.h"
#include "checks/UnregisteredUnitTestCheck.h"
#include "checks/UnregisteredWebRelayCheck.h"
#include "checks/UnrestrictedWebviewNavigationCheck.h"
#include "checks/UntitledControlCheck.h"
#include "checks/UseAfterCallbackCheck.h"
#include "checks/ValuetreeIdentityComparisonCheck.h"
#include "checks/ValuetreeListenerReentrancyCheck.h"
#include "checks/WebviewDevServerInReleaseCheck.h"
#include "checks/WebviewEventBeforeLoadCheck.h"
#include "checks/WebviewResourceMimeTypeCheck.h"
#include "checks/WebviewWindowsOptionsCheck.h"

#include <clang-tidy/ClangTidyModule.h>

namespace clang::tidy::juce
{
namespace
{
class JuceTidyModule : public ClangTidyModule
{
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override
    {
        factories.registerCheck<ApvtsStateAccessCheck>("juce-apvts-state-access");
        factories.registerCheck<AsyncThisCaptureCheck>("juce-async-this-capture");
        factories.registerCheck<AtomicInSampleLoopCheck>("juce-atomic-in-sample-loop");
        factories.registerCheck<AttachmentOrderCheck>("juce-attachment-order");
        factories.registerCheck<BlockSizeAssumptionCheck>("juce-block-size-assumption");
        factories.registerCheck<BufferPointerInSampleLoopCheck>("juce-buffer-pointer-in-sample-loop");
        factories.registerCheck<BufferedParentCheck>("juce-buffered-parent");
        factories.registerCheck<CallbackDanglingCaptureCheck>("juce-callback-dangling-capture");
        factories.registerCheck<DuplicateWebviewNameCheck>("juce-duplicate-webview-name");
        factories.registerCheck<EmptyStateInformationCheck>("juce-empty-state-information");
        factories.registerCheck<ExpensivePaintCheck>("juce-expensive-paint");
        factories.registerCheck<ExplicitBaseAccessCheck>("juce-explicit-base-access");
        factories.registerCheck<ExplicitNullptrComparisonCheck>("juce-explicit-nullptr-comparison");
        factories.registerCheck<ExplicitUnsignedIntCheck>("juce-explicit-unsigned-int");
        factories.registerCheck<FloatEqualityCheck>("juce-float-equality");
        factories.registerCheck<FloatMaxGeometryCheck>("juce-float-max-geometry");
        factories.registerCheck<GraphicsSaveStateCheck>("juce-graphics-save-state");
        factories.registerCheck<HardcodedResourceProviderRootCheck>("juce-hardcoded-resource-provider-root");
        factories.registerCheck<IdentifierFromLiteralCheck>("juce-identifier-from-literal");
        factories.registerCheck<IgnoreUnusedCheck>("juce-ignore-unused");
        factories.registerCheck<LeakDetectorCheck>("juce-leak-detector");
        factories.registerCheck<ListenerOnCopyCheck>("juce-listener-on-copy");
        factories.registerCheck<LocalFileChooserCheck>("juce-local-file-chooser");
        factories.registerCheck<LookAndFeelResetCheck>("juce-look-and-feel-reset");
        factories.registerCheck<MacroConditionStyleCheck>("juce-macro-condition-style");
        factories.registerCheck<ManualDeletionCheck>("juce-manual-deletion");
        factories.registerCheck<ManualResizedCallCheck>("juce-manual-resized-call");
        factories.registerCheck<MemberAccessOrderCheck>("juce-member-access-order");
        factories.registerCheck<MessageManagerLockUncheckedCheck>("juce-message-manager-lock-unchecked");
        factories.registerCheck<MessageThreadObjectInPrepareCheck>("juce-message-thread-object-in-prepare");
        factories.registerCheck<MissingAccessibilityHandlerCheck>("juce-missing-accessibility-handler");
        factories.registerCheck<MissingRemoveListenerCheck>("juce-missing-remove-listener");
        factories.registerCheck<NativeFunctionCompletionCheck>("juce-native-function-completion");
        factories.registerCheck<NonAsciiStringLiteralCheck>("juce-non-ascii-string-literal");
        factories.registerCheck<NormalisedParameterValueCheck>("juce-normalised-parameter-value");
        factories.registerCheck<NullableDereferenceCheck>("juce-nullable-dereference");
        factories.registerCheck<NumericLiteralsCheck>("juce-numeric-literals");
        factories.registerCheck<OpaqueWithoutFillCheck>("juce-opaque-without-fill");
        factories.registerCheck<OpenGlDetachCheck>("juce-opengl-detach");
        factories.registerCheck<PaintSideEffectsCheck>("juce-paint-side-effects");
        factories.registerCheck<ParameterIdLimitsCheck>("juce-parameter-id-limits");
        factories.registerCheck<ParameterVersionHintCheck>("juce-parameter-version-hint");
        factories.registerCheck<PassSmallObjectsByValueCheck>("juce-pass-small-objects-by-value");
        factories.registerCheck<PlatformMacrosCheck>("juce-platform-macros");
        factories.registerCheck<PluginGlobalStateCheck>("juce-plugin-global-state");
        factories.registerCheck<PreferConstexprCheck>("juce-prefer-constexpr");
        factories.registerCheck<PreferJassertCheck>("juce-prefer-jassert");
        factories.registerCheck<PreferLoggerCheck>("juce-prefer-logger");
        factories.registerCheck<PreferPreIncrementCheck>("juce-prefer-pre-increment");
        factories.registerCheck<PreferStartTimerHzCheck>("juce-prefer-start-timer-hz");
        factories.registerCheck<PreferStdContainersCheck>("juce-prefer-std-containers");
        factories.registerCheck<PreferTimedCallbackCheck>("juce-prefer-timed-callback");
        factories.registerCheck<PrivateCallbackInheritanceCheck>("juce-private-callback-inheritance");
        factories.registerCheck<ProcessorEditorCouplingCheck>("juce-processor-editor-coupling");
        factories.registerCheck<ProcessorStateFromEditorCheck>("juce-processor-state-from-editor");
        factories.registerCheck<ProgramCountCheck>("juce-program-count");
        factories.registerCheck<PropertyNameCaseCheck>("juce-property-name-case");
        factories.registerCheck<RealtimeSafetyCheck>("juce-realtime-safety");
        factories.registerCheck<RelativeFilePathCheck>("juce-relative-file-path");
        factories.registerCheck<RepaintTimerCheck>("juce-repaint-timer");
        factories.registerCheck<ScopedNoDenormalsCheck>("juce-scoped-no-denormals");
        factories.registerCheck<SeparatorCommentsCheck>("juce-separator-comments");
        factories.registerCheck<StateTypeCheck>("juce-state-type-check");
        factories.registerCheck<StdMathFunctionsCheck>("juce-std-math-functions");
        factories.registerCheck<StopInDestructorCheck>("juce-stop-in-destructor");
        factories.registerCheck<TemplateParameterNameCheck>("juce-template-parameter-name");
        factories.registerCheck<ThreadShouldExitCheck>("juce-thread-should-exit");
        factories.registerCheck<UnaddedChildComponentCheck>("juce-unadded-child-component");
        factories.registerCheck<UnboundCachedValueCheck>("juce-unbound-cached-value");
        factories.registerCheck<UncheckedChildTreeCheck>("juce-unchecked-child-tree");
        factories.registerCheck<UncheckedSampleRateCheck>("juce-unchecked-sample-rate");
        factories.registerCheck<UndoManagerConsistencyCheck>("juce-undo-manager-consistency");
        factories.registerCheck<UnitTestAssertionsCheck>("juce-unit-test-assertions");
        factories.registerCheck<UnknownParameterIdCheck>("juce-unknown-parameter-id");
        factories.registerCheck<UnownedChildComponentCheck>("juce-unowned-child-component");
        factories.registerCheck<UnpreparedDspCheck>("juce-unprepared-dsp");
        factories.registerCheck<UnregisteredUnitTestCheck>("juce-unregistered-unit-test");
        factories.registerCheck<UnregisteredWebRelayCheck>("juce-unregistered-web-relay");
        factories.registerCheck<UnrestrictedWebviewNavigationCheck>("juce-unrestricted-webview-navigation");
        factories.registerCheck<UntitledControlCheck>("juce-untitled-control");
        factories.registerCheck<UseAfterCallbackCheck>("juce-use-after-callback");
        factories.registerCheck<ValuetreeIdentityComparisonCheck>("juce-valuetree-identity-comparison");
        factories.registerCheck<ValuetreeListenerReentrancyCheck>("juce-valuetree-listener-reentrancy");
        factories.registerCheck<WebviewDevServerInReleaseCheck>("juce-webview-dev-server-in-release");
        factories.registerCheck<WebviewEventBeforeLoadCheck>("juce-webview-event-before-load");
        factories.registerCheck<WebviewResourceMimeTypeCheck>("juce-webview-resource-mime-type");
        factories.registerCheck<WebviewWindowsOptionsCheck>("juce-webview-windows-options");
    }
};
} // namespace

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): clang-tidy discovers plugin modules through this static registration
[[maybe_unused]] static const ClangTidyModuleRegistry::Add<JuceTidyModule> juceTidyModule("juce-module", "Adds checks for JUCE projects.");
} // namespace clang::tidy::juce
