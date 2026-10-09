#include "checks/ApvtsStateAccessCheck.h"
#include "checks/AsyncThisCaptureCheck.h"
#include "checks/AttachmentOrderCheck.h"
#include "checks/BlockSizeAssumptionCheck.h"
#include "checks/BufferPointerInSampleLoopCheck.h"
#include "checks/CallbackDanglingCaptureCheck.h"
#include "checks/ExpensivePaintCheck.h"
#include "checks/ExplicitBaseAccessCheck.h"
#include "checks/ExplicitNullptrComparisonCheck.h"
#include "checks/ExplicitUnsignedIntCheck.h"
#include "checks/GraphicsSaveStateCheck.h"
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
#include "checks/MissingRemoveListenerCheck.h"
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
#include "checks/PreferConstexprCheck.h"
#include "checks/PreferJassertCheck.h"
#include "checks/PreferPreIncrementCheck.h"
#include "checks/PropertyNameCaseCheck.h"
#include "checks/RealtimeSafetyCheck.h"
#include "checks/RelativeFilePathCheck.h"
#include "checks/ScopedNoDenormalsCheck.h"
#include "checks/StateTypeCheck.h"
#include "checks/StdMathFunctionsCheck.h"
#include "checks/StopInDestructorCheck.h"
#include "checks/TemplateParameterNameCheck.h"
#include "checks/ThreadShouldExitCheck.h"
#include "checks/UnaddedChildComponentCheck.h"
#include "checks/UnboundCachedValueCheck.h"
#include "checks/UncheckedChildTreeCheck.h"
#include "checks/UndoManagerConsistencyCheck.h"
#include "checks/UnknownParameterIdCheck.h"
#include "checks/UnownedChildComponentCheck.h"
#include "checks/UnpreparedDspCheck.h"
#include "checks/ValuetreeIdentityComparisonCheck.h"

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
        factories.registerCheck<AttachmentOrderCheck>("juce-attachment-order");
        factories.registerCheck<BlockSizeAssumptionCheck>("juce-block-size-assumption");
        factories.registerCheck<BufferPointerInSampleLoopCheck>("juce-buffer-pointer-in-sample-loop");
        factories.registerCheck<CallbackDanglingCaptureCheck>("juce-callback-dangling-capture");
        factories.registerCheck<ExpensivePaintCheck>("juce-expensive-paint");
        factories.registerCheck<ExplicitBaseAccessCheck>("juce-explicit-base-access");
        factories.registerCheck<ExplicitNullptrComparisonCheck>("juce-explicit-nullptr-comparison");
        factories.registerCheck<ExplicitUnsignedIntCheck>("juce-explicit-unsigned-int");
        factories.registerCheck<GraphicsSaveStateCheck>("juce-graphics-save-state");
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
        factories.registerCheck<MissingRemoveListenerCheck>("juce-missing-remove-listener");
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
        factories.registerCheck<PreferConstexprCheck>("juce-prefer-constexpr");
        factories.registerCheck<PreferJassertCheck>("juce-prefer-jassert");
        factories.registerCheck<PreferPreIncrementCheck>("juce-prefer-pre-increment");
        factories.registerCheck<PropertyNameCaseCheck>("juce-property-name-case");
        factories.registerCheck<RealtimeSafetyCheck>("juce-realtime-safety");
        factories.registerCheck<RelativeFilePathCheck>("juce-relative-file-path");
        factories.registerCheck<ScopedNoDenormalsCheck>("juce-scoped-no-denormals");
        factories.registerCheck<StateTypeCheck>("juce-state-type-check");
        factories.registerCheck<StdMathFunctionsCheck>("juce-std-math-functions");
        factories.registerCheck<StopInDestructorCheck>("juce-stop-in-destructor");
        factories.registerCheck<TemplateParameterNameCheck>("juce-template-parameter-name");
        factories.registerCheck<ThreadShouldExitCheck>("juce-thread-should-exit");
        factories.registerCheck<UnaddedChildComponentCheck>("juce-unadded-child-component");
        factories.registerCheck<UnboundCachedValueCheck>("juce-unbound-cached-value");
        factories.registerCheck<UncheckedChildTreeCheck>("juce-unchecked-child-tree");
        factories.registerCheck<UndoManagerConsistencyCheck>("juce-undo-manager-consistency");
        factories.registerCheck<UnknownParameterIdCheck>("juce-unknown-parameter-id");
        factories.registerCheck<UnownedChildComponentCheck>("juce-unowned-child-component");
        factories.registerCheck<UnpreparedDspCheck>("juce-unprepared-dsp");
        factories.registerCheck<ValuetreeIdentityComparisonCheck>("juce-valuetree-identity-comparison");
    }
};
} // namespace

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): clang-tidy discovers plugin modules through this static registration
[[maybe_unused]] static const ClangTidyModuleRegistry::Add<JuceTidyModule> juceTidyModule("juce-module", "Adds checks for JUCE projects.");
} // namespace clang::tidy::juce
