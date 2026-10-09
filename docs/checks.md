# Checks

- [juce-apvts-state-access](#juce-apvts-state-access)
- [juce-async-this-capture](#juce-async-this-capture)
- [juce-attachment-order](#juce-attachment-order)
- [juce-block-size-assumption](#juce-block-size-assumption)
- [juce-buffer-pointer-in-sample-loop](#juce-buffer-pointer-in-sample-loop)
- [juce-callback-dangling-capture](#juce-callback-dangling-capture)
- [juce-expensive-paint](#juce-expensive-paint)
- [juce-explicit-base-access](#juce-explicit-base-access)
- [juce-explicit-nullptr-comparison](#juce-explicit-nullptr-comparison)
- [juce-explicit-unsigned-int](#juce-explicit-unsigned-int)
- [juce-graphics-save-state](#juce-graphics-save-state)
- [juce-identifier-from-literal](#juce-identifier-from-literal)
- [juce-ignore-unused](#juce-ignore-unused)
- [juce-leak-detector](#juce-leak-detector)
- [juce-listener-on-copy](#juce-listener-on-copy)
- [juce-local-file-chooser](#juce-local-file-chooser)
- [juce-look-and-feel-reset](#juce-look-and-feel-reset)
- [juce-macro-condition-style](#juce-macro-condition-style)
- [juce-manual-deletion](#juce-manual-deletion)
- [juce-manual-resized-call](#juce-manual-resized-call)
- [juce-member-access-order](#juce-member-access-order)
- [juce-message-manager-lock-unchecked](#juce-message-manager-lock-unchecked)
- [juce-missing-remove-listener](#juce-missing-remove-listener)
- [juce-non-ascii-string-literal](#juce-non-ascii-string-literal)
- [juce-normalised-parameter-value](#juce-normalised-parameter-value)
- [juce-nullable-dereference](#juce-nullable-dereference)
- [juce-numeric-literals](#juce-numeric-literals)
- [juce-opaque-without-fill](#juce-opaque-without-fill)
- [juce-opengl-detach](#juce-opengl-detach)
- [juce-paint-side-effects](#juce-paint-side-effects)
- [juce-parameter-id-limits](#juce-parameter-id-limits)
- [juce-parameter-version-hint](#juce-parameter-version-hint)
- [juce-pass-small-objects-by-value](#juce-pass-small-objects-by-value)
- [juce-platform-macros](#juce-platform-macros)
- [juce-prefer-constexpr](#juce-prefer-constexpr)
- [juce-prefer-jassert](#juce-prefer-jassert)
- [juce-prefer-pre-increment](#juce-prefer-pre-increment)
- [juce-property-name-case](#juce-property-name-case)
- [juce-realtime-safety](#juce-realtime-safety)
- [juce-relative-file-path](#juce-relative-file-path)
- [juce-scoped-no-denormals](#juce-scoped-no-denormals)
- [juce-state-type-check](#juce-state-type-check)
- [juce-std-math-functions](#juce-std-math-functions)
- [juce-stop-in-destructor](#juce-stop-in-destructor)
- [juce-template-parameter-name](#juce-template-parameter-name)
- [juce-thread-should-exit](#juce-thread-should-exit)
- [juce-unadded-child-component](#juce-unadded-child-component)
- [juce-unbound-cached-value](#juce-unbound-cached-value)
- [juce-unchecked-child-tree](#juce-unchecked-child-tree)
- [juce-undo-manager-consistency](#juce-undo-manager-consistency)
- [juce-unknown-parameter-id](#juce-unknown-parameter-id)
- [juce-unowned-child-component](#juce-unowned-child-component)
- [juce-unprepared-dsp](#juce-unprepared-dsp)
- [juce-valuetree-identity-comparison](#juce-valuetree-identity-comparison)

## juce-apvts-state-access

Flags `getStateInformation` and `setStateInformation` using an `AudioProcessorValueTreeState`'s `state` directly. Hosts can call these from any thread, so use the thread-safe `copyState()` and `replaceState()` instead.

```cpp
auto xml = apvts.state.createXml(); // warning; use apvts.copyState().createXml()
```

## juce-async-this-capture

Flags lambdas that capture `this` and are passed to functions that call them later, such as `MessageManager::callAsync`. If the object is deleted first, `this` dangles. Capture a `SafePointer` or `WeakReference` instead.

```cpp
juce::MessageManager::callAsync([this] { // warning
    repaint();
});

juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer(this)] {
    if (safeThis != nullptr)
        safeThis->repaint();
});
```

See [juce-async-this-capture options](options.md#juce-async-this-capture).

## juce-attachment-order

Flags parameter attachments declared before the component they're attached to. Members are destroyed in reverse order, so the attachment outlives its component and crashes when it removes its listener.

```cpp
juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment; // warning
juce::Slider gainSlider;
```

## juce-block-size-assumption

Flags audio callbacks that use the block size saved in `prepareToPlay`. That's only the maximum, and hosts can pass fewer samples, so use the buffer's sample count.

```cpp
for (int i = 0; i < blockSize; ++i) // warning; use buffer.getNumSamples()
```

## juce-buffer-pointer-in-sample-loop

Flags `getWritePointer` and `getReadPointer` called on every iteration of a loop, and `getSample`, `setSample` and `addSample` called per sample. Getting the pointer once is faster.

```cpp
for (int i = 0; i < buffer.getNumSamples(); ++i)
    buffer.getWritePointer(channel)[i] *= gain; // warning
```

## juce-callback-dangling-capture

Flags lambdas assigned to callbacks like `onClick` that capture locals by reference. The callback runs after the function returns, when the locals no longer exist.

```cpp
int clicks = 0;
button.onClick = [&] { // warning
    ++clicks;
};
```

## juce-expensive-paint

Flags expensive work in `paint`, such as loading images, parsing XML or JSON, creating typefaces and file access. It's repeated on every repaint, so do it once and store the result.

```cpp
void paint(juce::Graphics& g) override
{
    auto logo = juce::ImageFileFormat::loadFrom(logoFile); // warning
}
```

## juce-explicit-base-access

Flags base classes without an access specifier, which the JUCE coding standards require.

```cpp
class Editor : juce::Component // class Editor : private juce::Component
```

## juce-explicit-nullptr-comparison

Flags smart pointers and `std::function`s used as booleans. The JUCE coding standards ask for an explicit comparison with `nullptr`.

```cpp
if (onClick) // if (onClick != nullptr)
if (!owned)  // if (owned == nullptr)
```

See [juce-explicit-nullptr-comparison options](options.md#juce-explicit-nullptr-comparison).

## juce-explicit-unsigned-int

Flags `unsigned` on its own. The JUCE coding standards ask for `unsigned int`.

```cpp
unsigned count; // unsigned int count;
```

## juce-graphics-save-state

Flags `Graphics::saveState()` without a matching `restoreState()`, which leaves the state modified for whatever's painted next. Use `ScopedSaveState` instead.

```cpp
g.saveState(); // warning
const juce::Graphics::ScopedSaveState state(g);
```

## juce-identifier-from-literal

Flags `juce::Identifier`s created from string literals every time code runs. Creating an Identifier is slow, so keep a static one.

```cpp
tree.getProperty("gain"); // warning

static const juce::Identifier gainId("gain");
tree.getProperty(gainId);
```

## juce-ignore-unused

Flags `juce::ignoreUnused`. `[[maybe_unused]]` does the same job, and is what JUCE now uses.

```cpp
void process(int channel) // void process([[maybe_unused]] int channel)
{
    juce::ignoreUnused(channel);
}
```

## juce-leak-detector

Flags `Component` and `AudioProcessor` subclasses without a leak detector, which JUCE's coding standards and project templates recommend.

```cpp
class Editor : public juce::Component // warning
{
};
```

See [juce-leak-detector options](options.md#juce-leak-detector).

## juce-listener-on-copy

Flags listeners added to a local or temporary `ValueTree` or `Value`. The listener belongs to that object, so it's lost when the copy goes away.

```cpp
auto tree = apvts.state;
tree.addListener(this); // warning
```

## juce-local-file-chooser

Flags a local `FileChooser` used with `launchAsync`. Deleting the chooser cancels the dialog, so keep it in a member.

```cpp
juce::FileChooser chooser("Open");
chooser.launchAsync(flags, callback); // warning
```

## juce-look-and-feel-reset

Flags `setLookAndFeel` calls that aren't undone with `nullptr` in the destructor. JUCE asserts when a LookAndFeel is deleted while a component still uses it.

```cpp
MyEditor::MyEditor()
{
    slider.setLookAndFeel(&lookAndFeel);
}

MyEditor::~MyEditor()
{
    slider.setLookAndFeel(nullptr);
}
```

## juce-macro-condition-style

Flags `#ifdef`, `defined(…)` and `== 1` checks on JUCE's 0-or-1 flag macros. They're always defined, so `#ifdef` is true even when they're 0. It conflicts with the built-in `readability-use-concise-preprocessor-directives`.

```cpp
#ifdef JucePlugin_Build_VST3 // warning: use '#if JucePlugin_Build_VST3'
#if JucePlugin_Build_AU == 1 // warning: use '#if JucePlugin_Build_AU'
```

See [juce-macro-condition-style options](options.md#juce-macro-condition-style).

## juce-manual-deletion

Flags `deleteAndZero` and `deleteAllChildren`. The JUCE coding standards say never to delete objects manually, so use members, `std::unique_ptr` or `OwnedArray`.

```cpp
deleteAllChildren(); // warning
```

## juce-manual-resized-call

Flags direct calls to `resized()`. Put the layout code in a separate method and call that instead.

```cpp
void resized() override
{
    updateLayout();
}

void setCompactMode(bool compact)
{
    isCompact = compact;
    resized(); // updateLayout();
}
```

See [juce-manual-resized-call options](options.md#juce-manual-resized-call).

## juce-member-access-order

Flags classes that don't declare public members first, then protected, then private, as the JUCE coding standards ask.

```cpp
class Meter
{
    float level = 0.0f;

public: // warning
    float getLevel() const;
};
```

## juce-message-manager-lock-unchecked

Flags a `MessageManagerLock` given a thread to watch when `lockWasGained()` is never checked. If the thread is told to exit, the lock returns without locking.

```cpp
const juce::MessageManagerLock lock(this);

if (!lock.lockWasGained())
    return;
```

## juce-missing-remove-listener

Flags `add…Listener(this)` on an object the class doesn't own when the destructor never calls the matching `remove…Listener`. Otherwise the broadcaster can call a destroyed object.

```cpp
Editor(Processor& p)
{
    p.addChangeListener(this); // warning
}

~Editor() override {}
```

## juce-non-ascii-string-literal

Flags non-ASCII string literals converted to a `juce::String`. `String` assumes `const char*` is ASCII and asserts otherwise.

```cpp
label.setText("90°", juce::dontSendNotification);
// label.setText(juce::String::fromUTF8("90°"), juce::dontSendNotification);
```

## juce-normalised-parameter-value

Flags `getValue()` on a `RangedAudioParameter`. It returns the normalised 0 to 1 value, which is easy to mistake for the real value.

```cpp
auto cutoff = apvts.getParameter("cutoff")->getValue(); // warning
```

## juce-nullable-dereference

Flags dereferencing JUCE functions that can return `nullptr`, such as `getPlayHead()` when the host has no play head.

```cpp
const auto bpm = getPlayHead()->getPosition()->getBpm(); // warning

if (auto* playHead = getPlayHead())
{
    // ...
}
```

See [juce-nullable-dereference options](options.md#juce-nullable-dereference).

## juce-numeric-literals

Flags floating-point literals without a digit on both sides of the point, and uppercase hex literals, following the JUCE coding standards.

```cpp
auto half = .5f;        // auto half = 0.5f;
auto mask = 0xFF00FF00; // auto mask = 0xff00ff00;
```

## juce-opaque-without-fill

Flags opaque components whose `paint` doesn't fill them. JUCE doesn't draw what's behind an opaque component, so unpainted areas show leftover pixels.

```cpp
setOpaque(true); // warning: paint() never fills the component
```

## juce-opengl-detach

Flags components that attach an `OpenGLContext` but don't detach it in the destructor. JUCE requires this so the render thread stops safely.

```cpp
MyComponent::~MyComponent()
{
    openGLContext.detach();
}
```

## juce-paint-side-effects

Flags `repaint()`, `setBounds`, `addAndMakeVisible` and `setVisible` in `paint`. Each triggers another repaint, which can loop forever.

```cpp
void paint(juce::Graphics& g) override
{
    repaint(); // warning
}
```

## juce-parameter-id-limits

Flags parameter IDs over 31 characters, which AAX doesn't support, and duplicate IDs. JUCE asserts on both.

```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, ...));
layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, ...)); // warning
```

## juce-parameter-version-hint

Flags parameters created without a version hint. AU hosts need them, and JUCE asserts in AU builds.

```cpp
std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 0.5f);
// std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain", 0.0f, 1.0f, 0.5f);
```

## juce-pass-small-objects-by-value

Flags `const&` parameters of small JUCE types that the JUCE coding standards say to pass by value.

```cpp
void setColour(const juce::Colour& colour); // void setColour(juce::Colour colour);
```

See [juce-pass-small-objects-by-value options](options.md#juce-pass-small-objects-by-value).

## juce-platform-macros

Flags compiler and platform macros like `_WIN32` and `NDEBUG` where JUCE has an equivalent. Using JUCE's macros keeps platform checks consistent and easy to search for.

```cpp
#ifdef _WIN32  // warning: use '#if JUCE_WINDOWS'
#ifndef NDEBUG // warning: use '#if JUCE_DEBUG'
```

## juce-prefer-constexpr

Flags `const` variables initialised with a constant expression. The JUCE coding standards say compile-time constants should be `constexpr`.

```cpp
const int numChannels = 2; // constexpr int numChannels = 2;
```

## juce-prefer-jassert

Flags `assert`, and `jassert(false)`. `jassert` breaks into the debugger and logs the failure, and `jassertfalse` says the same thing more clearly.

```cpp
assert(index >= 0); // jassert(index >= 0);
jassert(false);     // jassertfalse;
```

## juce-prefer-pre-increment

Flags `i++` and `i--` when the result isn't used. The JUCE coding standards say to use pre-increment wherever it would do.

```cpp
for (int i = 0; i < size; i++) // ++i
```

## juce-property-name-case

Flags identifiers and parameter IDs that differ from another in the same file only by case. JUCE compares them case-sensitively, so the typo silently uses a different property.

```cpp
static const juce::Identifier cutoffId{ "cutoff" };
state.setProperty("Cutoff", 1000, nullptr); // warning
```

## juce-realtime-safety

Flags allocation, locking, I/O, message-thread work and parameter lookups by ID in audio callbacks, parameter callbacks and every function they call. These can block the audio thread and cause dropouts.

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    DBG("processing");                         // warning
    samples.push_back(buffer.getSample(0, 0)); // warning: may allocate
}
```

Problems in called functions are reported where they happen, with notes showing how they're reached from the callback.

### Code in other files

clang-tidy can't see into functions defined in other `.cpp` files. Mark them `[[clang::nonblocking]]` so they're checked as entry points and trusted by callers. Set [`ReportUncheckedCalls`](options.md#juce-realtime-safety) to find the functions that need it.

```cpp
void process(juce::AudioBuffer<float>& buffer) noexcept [[clang::nonblocking]];
```

See [juce-realtime-safety options](options.md#juce-realtime-safety).

## juce-relative-file-path

Flags a `juce::File` created from a relative path literal. JUCE asserts, as the path depends on the working directory, which differs between hosts.

```cpp
juce::File presets("Presets/Default.xml"); // warning
```

## juce-scoped-no-denormals

Flags `processBlock` without a `juce::ScopedNoDenormals`. Denormal numbers can cause large CPU spikes.

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    juce::ScopedNoDenormals noDenormals;
}
```

See [juce-scoped-no-denormals options](options.md#juce-scoped-no-denormals).

## juce-state-type-check

Flags `setStateInformation` loading saved state into an `AudioProcessorValueTreeState` without checking its type. Old or corrupt data would replace every parameter.

```cpp
if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
```

## juce-std-math-functions

Flags C maths functions and JUCE functions with an exact `std::` equivalent. The JUCE coding standards say to use the `std::` versions.

```cpp
auto root = sqrtf(x);                       // auto root = std::sqrt(x);
auto clamped = juce::jlimit(0.0f, 1.0f, x); // auto clamped = std::clamp(x, 0.0f, 1.0f);
```

See [juce-std-math-functions options](options.md#juce-std-math-functions).

## juce-stop-in-destructor

Flags `Thread` and `HighResolutionTimer` subclasses that don't stop in their destructor. The callback can otherwise run while the object is being destroyed, and JUCE asserts.

```cpp
Worker::~Worker()
{
    stopThread(1000);
}
```

## juce-template-parameter-name

Flags very short template parameter names like `T`. The JUCE coding standards ask for descriptive names, partly to avoid clashes with macros.

```cpp
template <typename T> // warning
```

See [juce-template-parameter-name options](options.md#juce-template-parameter-name).

## juce-thread-should-exit

Flags endless loops in `Thread::run` that never check `threadShouldExit()`. `stopThread` will time out and kill the thread, which can leak resources or leave locks held.

```cpp
while (true)                // warning
while (!threadShouldExit()) // fine
```

## juce-unadded-child-component

Flags child component members that are never added with `addAndMakeVisible` or `addChildComponent`, so they never appear.

```cpp
juce::Slider gainSlider; // warning if never added
```

## juce-unbound-cached-value

Flags `CachedValue` members never attached to a `ValueTree` property. They always return their default, and JUCE asserts when they're written.

```cpp
juce::CachedValue<float> gain; // warning if nothing calls gain.referTo(tree, "gain", nullptr)
```

## juce-unchecked-child-tree

Flags modifying a `ValueTree` from `getChildWithName` and similar without checking it's valid. When there's no match the tree is invalid, and the change asserts and does nothing.

```cpp
state.getChildWithName("Filter").setProperty("cutoff", 1000, nullptr); // warning
state.getOrCreateChildWithName("Filter", nullptr).setProperty("cutoff", 1000, nullptr);
```

## juce-undo-manager-consistency

Flags `ValueTree` changes with a `nullptr` `UndoManager` in a class that elsewhere passes a real one. Mixing them usually means some changes were forgotten and can't be undone.

```cpp
void setCutoff(float cutoff)
{
    state.setProperty("cutoff", cutoff, &undoManager);
}

void setResonance(float resonance)
{
    state.setProperty("resonance", resonance, nullptr); // warning
}
```

## juce-unknown-parameter-id

Flags parameter lookups whose ID doesn't match any parameter created in the same file. A misspelt ID returns `nullptr` or makes the attachment assert.

```cpp
apvts.getRawParameterValue("gian"); // warning: did you mean 'gain'?
```

## juce-unowned-child-component

Flags components created with `new` and passed straight to `addAndMakeVisible` and similar. These don't take ownership, so the component leaks.

```cpp
addAndMakeVisible(new juce::Label()); // warning
```

## juce-unprepared-dsp

Flags `SmoothedValue`, `juce::dsp` processors, `Oversampling`, `ADSR` and `Reverb` members that the class never prepares. They don't work correctly until prepared with the sample rate.

```cpp
juce::dsp::Gain<float> gain; // warning if gain.prepare(spec) is never called
```

## juce-valuetree-identity-comparison

Flags `==` and `!=` between a `ValueTree` and a newly created one. `==` compares identity, not contents, so it's always `false`. Use `isEquivalentTo`.

```cpp
if (state == juce::ValueTree::fromXml(*xml)) // warning
if (state.isEquivalentTo(juce::ValueTree::fromXml(*xml)))
```
