# Checks

- [juce-apvts-state-access](#juce-apvts-state-access)
- [juce-async-this-capture](#juce-async-this-capture)
- [juce-atomic-in-sample-loop](#juce-atomic-in-sample-loop)
- [juce-attachment-order](#juce-attachment-order)
- [juce-block-size-assumption](#juce-block-size-assumption)
- [juce-buffer-pointer-in-sample-loop](#juce-buffer-pointer-in-sample-loop)
- [juce-buffered-parent](#juce-buffered-parent)
- [juce-callback-dangling-capture](#juce-callback-dangling-capture)
- [juce-duplicate-webview-name](#juce-duplicate-webview-name)
- [juce-expensive-paint](#juce-expensive-paint)
- [juce-explicit-base-access](#juce-explicit-base-access)
- [juce-explicit-nullptr-comparison](#juce-explicit-nullptr-comparison)
- [juce-explicit-unsigned-int](#juce-explicit-unsigned-int)
- [juce-graphics-save-state](#juce-graphics-save-state)
- [juce-hardcoded-resource-provider-root](#juce-hardcoded-resource-provider-root)
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
- [juce-missing-accessibility-handler](#juce-missing-accessibility-handler)
- [juce-missing-remove-listener](#juce-missing-remove-listener)
- [juce-native-function-completion](#juce-native-function-completion)
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
- [juce-plugin-global-state](#juce-plugin-global-state)
- [juce-prefer-constexpr](#juce-prefer-constexpr)
- [juce-prefer-jassert](#juce-prefer-jassert)
- [juce-prefer-pre-increment](#juce-prefer-pre-increment)
- [juce-processor-editor-coupling](#juce-processor-editor-coupling)
- [juce-property-name-case](#juce-property-name-case)
- [juce-realtime-safety](#juce-realtime-safety)
- [juce-relative-file-path](#juce-relative-file-path)
- [juce-repaint-timer](#juce-repaint-timer)
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
- [juce-unregistered-web-relay](#juce-unregistered-web-relay)
- [juce-unrestricted-webview-navigation](#juce-unrestricted-webview-navigation)
- [juce-untitled-control](#juce-untitled-control)
- [juce-valuetree-identity-comparison](#juce-valuetree-identity-comparison)
- [juce-webview-dev-server-in-release](#juce-webview-dev-server-in-release)
- [juce-webview-event-before-load](#juce-webview-event-before-load)
- [juce-webview-resource-mime-type](#juce-webview-resource-mime-type)
- [juce-webview-windows-options](#juce-webview-windows-options)

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

## juce-atomic-in-sample-loop

Flags atomics and parameters read on every iteration of a loop in an audio callback, such as `std::atomic::load()` or `juce::AudioParameterFloat::get()`. Each read has a cost that adds up over every sample in the block. Read the value once before the loop instead.

```cpp
for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    data[sample] *= gain.load(); // warning

const auto currentGain = gain.load();

for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    data[sample] *= currentGain;
```

Reads that depend on the loop variable, such as `voiceGains[voice].load()`, are allowed.

See [juce-atomic-in-sample-loop options](options.md#juce-atomic-in-sample-loop).

## juce-attachment-order

Flags parameter attachments declared before the component or web relay they're attached to, and web relays declared after the `WebBrowserComponent` they're registered with. Members are destroyed in reverse order, so the attachment outlives its component and crashes when it removes its listener, and the browser calls back into relays that no longer exist.

```cpp
juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment; // warning
juce::Slider gainSlider;

juce::WebBrowserComponent webview{ juce::WebBrowserComponent::Options{}.withOptionsFrom(cutoffRelay) };
juce::WebSliderRelay cutoffRelay{ "cutoff" }; // warning
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

## juce-buffered-parent

Flags `setBufferedToImage(true)` on a component that also has child components. Whenever a child repaints, the parent has to redraw its cached image too, so the cache saves nothing. Buffer a sibling component that does the drawing instead.

```cpp
setBufferedToImage(true); // warning
addAndMakeVisible(foreground);

background.setBufferedToImage(true);
addAndMakeVisible(background);
addAndMakeVisible(foreground);
```

## juce-callback-dangling-capture

Flags lambdas assigned to callbacks like `onClick`, or passed to `WebBrowserComponent` functions like `withNativeFunction` and `evaluateJavascript`, that capture locals by reference. The callback runs after the function returns, when the locals no longer exist.

```cpp
int clicks = 0;
button.onClick = [&] { // warning
    ++clicks;
};
```

## juce-duplicate-webview-name

Flags native functions registered twice with the same name, and relays of the same type in one class that share a name. JUCE asserts on duplicate native functions and only keeps the last one, and relays that share a name are connected to the same frontend state.

```cpp
juce::WebSliderRelay gainRelay{ "gain" };
juce::WebSliderRelay outputRelay{ "gain" }; // warning
```

## juce-expensive-paint

Flags expensive work in `paint`, such as loading images, parsing XML or JSON, creating typefaces and file access. It's repeated on every repaint, so do it once and store the result.

It also flags a `juce::Path` or `juce::ColourGradient` built from the same values on every repaint, such as one that doesn't depend on the component's size or state. Build it once and store it as a member.

```cpp
void paint(juce::Graphics& g) override
{
    auto logo = juce::ImageFileFormat::loadFrom(logoFile); // warning

    juce::Path arrow; // warning
    arrow.startNewSubPath(0.0f, 0.0f);
    arrow.lineTo(10.0f, 5.0f);
    arrow.lineTo(0.0f, 10.0f);
    g.fillPath(arrow);
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

## juce-hardcoded-resource-provider-root

Flags string literals containing `juce://juce.backend` or `https://juce.backend`. The resource provider root is different on Windows and Android, so use `getResourceProviderRoot()`. Where the literal is used as a `juce::String`, the warning includes a fix.

```cpp
webview.goToURL("juce://juce.backend/");           // webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
webview.goToURL("juce://juce.backend/index.html"); // webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot() + "index.html");
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

## juce-missing-accessibility-handler

Flags components that override `mouseDown`, `mouseUp`, `mouseDrag` or `mouseDoubleClick` but don't override `createAccessibilityHandler()`. The default handler has no role, value or actions, so screen readers can't tell what the component is or interact with it. Components that inherit a handler, such as `Slider` subclasses, and components hidden with `setAccessible(false)` are ignored.

```cpp
class XYPad : public juce::Component // warning
{
public:
    void mouseDrag(const juce::MouseEvent& event) override;
};
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

## juce-native-function-completion

Flags native functions that never call their completion. The completion resolves the promise returned to JavaScript, so without it the call never finishes.

```cpp
options.withNativeFunction("save", [](auto& args, auto complete) { // warning
    save(args[0]);
});
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

## juce-plugin-global-state

Flags singletons declared with `JUCE_DECLARE_SINGLETON`, and mutable static data members and static local variables. Depending on the host, plugin instances may or may not share a process, so state like this may or may not be shared between them, and `thread_local` doesn't help. Store the state per instance, or use a `juce::SharedResourcePointer` if it really should be shared.

```cpp
juce::String getPluginName()
{
    static juce::String name; // warning
    return name;
}
```

Global variables at namespace scope aren't flagged, as `cppcoreguidelines-avoid-non-const-global-variables` already covers them. Disable this check for apps, where there's only ever one instance.

See [juce-plugin-global-state options](options.md#juce-plugin-global-state).

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

## juce-processor-editor-coupling

Flags processors that call into their editor with `getActiveEditor()`, and editors that give the processor a lambda capturing `this`, or a pointer to themselves. There may be no editor, or several, and the editor can be destroyed while the processor still holds on to it. Have the editor read the processor's state instead, for example from a `juce::Timer` or a listener.

```cpp
void Processor::loadPreset()
{
    if (auto* editor = getActiveEditor()) // warning
        editor->repaint();
}

Editor::Editor(Processor& p)
    : juce::AudioProcessorEditor{ p }
{
    p.onPresetLoaded = [this] { // warning
        repaint();
    };
}
```

Assignments that are reset in the editor's destructor are allowed.

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

## juce-repaint-timer

Flags `timerCallback()` overrides that only call `repaint()` on a timer running at 30Hz or more. A `juce::Timer` doesn't run in time with the display, so use a `juce::VBlankAttachment` to repaint on every frame instead.

```cpp
Meter()
{
    startTimerHz(60);
}

void timerCallback() override // warning
{
    repaint();
}

juce::VBlankAttachment vBlank{ this, [this] { repaint(); } };
```

See [juce-repaint-timer options](options.md#juce-repaint-timer).

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

## juce-unregistered-web-relay

Flags `WebSliderRelay`, `WebToggleButtonRelay`, `WebComboBoxRelay` and `WebControlParameterIndexReceiver` members that are never passed to `withOptionsFrom`. They're never connected to the browser, so the frontend never receives their state.

```cpp
juce::WebSliderRelay gainRelay{ "gain" }; // warning if nothing calls withOptionsFrom(gainRelay)
```

## juce-unrestricted-webview-navigation

Flags a `WebBrowserComponent` with native integration enabled whose class doesn't override `pageAboutToLoad`. Any page it navigates to, such as through a link, can then call your native functions, so only allow your own pages.

```cpp
class SinglePageBrowser : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;

    bool pageAboutToLoad(const juce::String& url) override
    {
        return url == getResourceProviderRoot();
    }
};
```

## juce-untitled-control

Flags `Button`, `ComboBox`, `Slider` and `TextEditor` members that are never given a title with `setTitle()`, so screen readers announce them without saying what they control. Buttons can use their button text instead, set with `setButtonText()` or the constructor. A component's name, such as the one passed to `juce::Slider`'s constructor, isn't announced.

Members of your own subclasses of these controls are checked too, unless the subclass gives itself a title, by calling `setTitle()` or `setButtonText()` on itself or passing text to a button base class's constructor. Members passed to your own functions are assumed to be given a title there.

```cpp
juce::Slider gainSlider; // warning if 'gainSlider.setTitle()' is never called
juce::Slider cutoffSlider{ "Cutoff" }; // warning; the name isn't the title
juce::TextButton bypassButton{ "Bypass" };
```

## juce-valuetree-identity-comparison

Flags `==` and `!=` between a `ValueTree` and a newly created one. `==` compares identity, not contents, so it's always `false`. Use `isEquivalentTo`.

```cpp
if (state == juce::ValueTree::fromXml(*xml)) // warning
if (state.isEquivalentTo(juce::ValueTree::fromXml(*xml)))
```

## juce-webview-dev-server-in-release

Flags local development servers like `http://localhost:3000` passed to `goToURL` or allowed by `pageAboutToLoad`, and allowed origins passed to `withResourceProvider`, unless they're inside `#if JUCE_DEBUG`. JUCE warns that allowing other origins is a security risk in released builds.

```cpp
options.withResourceProvider(provide, juce::URL{ "http://localhost:3000" }.getOrigin()); // warning

#if JUCE_DEBUG
webview.goToURL("http://localhost:3000/");
#endif
```

See [juce-webview-dev-server-in-release options](options.md#juce-webview-dev-server-in-release).

## juce-webview-event-before-load

Flags `emitEventIfBrowserIsVisible` and `evaluateJavascript` called on a browser while the object that owns it is being constructed. The page can't have loaded yet, so the frontend misses the event and the script doesn't run in the page. Pass initial state with `withInitialisationData` instead, and run scripts with `withUserScript`.

```cpp
Editor()
{
    webview.emitEventIfBrowserIsVisible("ready", true); // warning
}
```

## juce-webview-resource-mime-type

Flags `WebBrowserComponent::Resource`s without a MIME type. Browsers won't run module scripts, like the ones Vite builds, unless they have a JavaScript MIME type.

```cpp
return juce::WebBrowserComponent::Resource{ .data = bytes }; // warning
return juce::WebBrowserComponent::Resource{ .data = bytes, .mimeType = "text/javascript" };
```

## juce-webview-windows-options

Flags options that use native integration, user scripts, a resource provider or relays without choosing the WebView2 backend, and the WebView2 backend without a user data folder. On Windows, JUCE otherwise falls back to Internet Explorer, which supports none of them, and WebView2 can fail if it can't write to the default folder next to the executable. Neither problem shows up on macOS.

```cpp
juce::WebBrowserComponent::Options{}
    .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)))
    .withNativeIntegrationEnabled();
```
