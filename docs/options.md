# Options

Options are set through `CheckOptions` in your `.clang-tidy`, for example `juce-template-parameter-name.MinimumLength: 3`. List options are semicolon-separated, and qualified names may start with `::`.

- [juce-async-this-capture](#juce-async-this-capture)
- [juce-atomic-in-sample-loop](#juce-atomic-in-sample-loop)
- [juce-explicit-nullptr-comparison](#juce-explicit-nullptr-comparison)
- [juce-leak-detector](#juce-leak-detector)
- [juce-macro-condition-style](#juce-macro-condition-style)
- [juce-manual-resized-call](#juce-manual-resized-call)
- [juce-nullable-dereference](#juce-nullable-dereference)
- [juce-pass-small-objects-by-value](#juce-pass-small-objects-by-value)
- [juce-plugin-global-state](#juce-plugin-global-state)
- [juce-realtime-safety](#juce-realtime-safety)
- [juce-repaint-timer](#juce-repaint-timer)
- [juce-scoped-no-denormals](#juce-scoped-no-denormals)
- [juce-std-math-functions](#juce-std-math-functions)
- [juce-template-parameter-name](#juce-template-parameter-name)
- [juce-webview-dev-server-in-release](#juce-webview-dev-server-in-release)

## juce-async-this-capture

| Option | Default |
| --- | --- |
| `AsyncFunctions`: functions that call their argument later | `juce::MessageManager::callAsync`, `juce::Timer::callAfterDelay`, `juce::Thread::launch`, `juce::NativeMessageBox::showAsync`, `juce::AlertWindow::showAsync` |
| `SafePointerTypes`: captures that make `this` safe to use | `juce::Component::SafePointer`, `juce::WeakReference`, `std::weak_ptr` |

## juce-atomic-in-sample-loop

| Option | Default |
| --- | --- |
| `AudioCallbacks`: functions that run on the audio thread, including their overrides | `AudioProcessor::processBlock`, `AudioProcessor::processBlockBypassed`, `AudioSource::getNextAudioBlock`, `AudioIODeviceCallback::audioDeviceIOCallbackWithContext`, `SynthesiserVoice::renderNextBlock`, `MPESynthesiserVoice::renderNextBlock`, `dsp::ProcessorBase::process` |

## juce-explicit-nullptr-comparison

| Option | Default |
| --- | --- |
| `SmartPointers`: types with an explicit `operator bool` to treat as pointers | `::std::unique_ptr;::std::shared_ptr;::std::function` |

## juce-leak-detector

| Option | Default |
| --- | --- |
| `BaseClasses`: classes whose subclasses need a leak detector | `::juce::Component;::juce::AudioProcessor` |
| `Macros`: macros that count as a leak detector | `JUCE_LEAK_DETECTOR;JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` |

## juce-macro-condition-style

| Option | Default |
| --- | --- |
| `FlagMacros`: a regular expression for the names of 0-or-1 flag macros | `^(JUCE\|JucePlugin)_` |

## juce-manual-resized-call

| Option | Default |
| --- | --- |
| `LayoutFunctionName`: the method to call instead | `updateLayout` |

## juce-nullable-dereference

| Option | Default |
| --- | --- |
| `NullableFunctions`: functions that may return `nullptr` | `AudioProcessor::getPlayHead`, `AudioProcessor::getActiveEditor`, `AudioProcessorValueTreeState::getParameter`, `AudioProcessorValueTreeState::getRawParameterValue`, `Component::getParentComponent`, `Component::findParentComponentOfClass`, `Component::getPeer`, `MessageManager::getInstanceWithoutCreating`, `AudioDeviceManager::getCurrentAudioDevice`, `Displays::getPrimaryDisplay`, `AudioProcessor::getXmlFromBinary`, `parseXML`, `parseXMLIfTagMatches`, `XmlDocument::parse`, `XmlDocument::getDocumentElement`, `ValueTree::createXml`, `WebBrowserComponent::EvaluationResult::getResult`, `WebBrowserComponent::EvaluationResult::getError` |

## juce-pass-small-objects-by-value

| Option | Default |
| --- | --- |
| `Types`: class names to pass by value | `juce::Point`, `Rectangle`, `Range`, `Colour`, `Time`, `RelativeTime`, `Identifier`, `ModifierKeys`, `Justification`, `PixelRGB`, `PixelARGB`, `PixelAlpha`, `StringRef` and the `CharPointer_*` classes |

## juce-plugin-global-state

| Option | Default |
| --- | --- |
| `SingletonMacros`: macros that declare a singleton | `JUCE_DECLARE_SINGLETON;JUCE_DECLARE_SINGLETON_SINGLETHREADED;JUCE_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL;JUCE_DECLARE_SINGLETON_INLINE` |

## juce-realtime-safety

| Option | Default |
| --- | --- |
| `AudioCallbacks`: functions that run on the audio thread, including their overrides | `AudioProcessor::processBlock`, `AudioProcessor::processBlockBypassed`, `AudioSource::getNextAudioBlock`, `AudioIODeviceCallback::audioDeviceIOCallbackWithContext`, `SynthesiserVoice::renderNextBlock`, `MPESynthesiserVoice::renderNextBlock`, `dsp::ProcessorBase::process` |
| `ParameterCallbacks`: parameter listeners that can run on the audio thread, including their overrides | `AudioProcessorValueTreeState::Listener::parameterChanged`, `AudioProcessorParameter::Listener::parameterValueChanged`, `AudioProcessorParameter::Listener::parameterGestureChanged`, `AudioProcessorListener::audioProcessorParameterChanged`, and `valueChanged` on `AudioParameterFloat`, `AudioParameterInt`, `AudioParameterBool` and `AudioParameterChoice` |
| `OpaqueNamespaces`: namespaces whose functions aren't followed | `::juce;::std` |
| `ReportUncheckedCalls`: warn about calls into functions that can't be followed | `false` |
| `UnsafeFunctions`: extra functions to flag | none |
| `UnsafeMacros`: macros to flag | `DBG` |
| `CheckParameterLookups`: flag parameter lookups by ID | `true` |

## juce-repaint-timer

| Option | Default |
| --- | --- |
| `MinimumFrequency`: the lowest timer rate, in Hz, to flag | `30` |

## juce-scoped-no-denormals

| Option | Default |
| --- | --- |
| `AudioCallbacks`: functions to check, including their overrides | `::juce::AudioProcessor::processBlock` |

## juce-std-math-functions

| Option | Default |
| --- | --- |
| `PreferJuceFunctions`: prefer JUCE's functions over their `std::` equivalents | `false` |

## juce-template-parameter-name

| Option | Default |
| --- | --- |
| `MinimumLength`: the shortest allowed name | `2` |

## juce-webview-dev-server-in-release

| Option | Default |
| --- | --- |
| `DebugMacros`: macros whose `#if` branches are only built for development | `JUCE_DEBUG` |
