# Options

Options are set through `CheckOptions` in your `.clang-tidy`, for example `juce-template-parameter-name.MinimumLength: 3`. List options are semicolon-separated, and qualified names may start with `::`.

- [juce-async-this-capture](#juce-async-this-capture)
- [juce-atomic-in-sample-loop](#juce-atomic-in-sample-loop)
- [juce-explicit-nullptr-comparison](#juce-explicit-nullptr-comparison)
- [juce-leak-detector](#juce-leak-detector)
- [juce-macro-condition-style](#juce-macro-condition-style)
- [juce-manual-resized-call](#juce-manual-resized-call)
- [juce-message-thread-object-in-prepare](#juce-message-thread-object-in-prepare)
- [juce-nullable-dereference](#juce-nullable-dereference)
- [juce-pass-small-objects-by-value](#juce-pass-small-objects-by-value)
- [juce-plugin-global-state](#juce-plugin-global-state)
- [juce-prefer-logger](#juce-prefer-logger)
- [juce-prefer-std-containers](#juce-prefer-std-containers)
- [juce-private-callback-inheritance](#juce-private-callback-inheritance)
- [juce-processor-state-from-editor](#juce-processor-state-from-editor)
- [juce-realtime-safety](#juce-realtime-safety)
- [juce-repaint-timer](#juce-repaint-timer)
- [juce-scoped-no-denormals](#juce-scoped-no-denormals)
- [juce-std-math-functions](#juce-std-math-functions)
- [juce-template-parameter-name](#juce-template-parameter-name)
- [juce-unchecked-sample-rate](#juce-unchecked-sample-rate)
- [juce-use-after-callback](#juce-use-after-callback)
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

## juce-message-thread-object-in-prepare

| Option | Default |
| --- | --- |
| `Functions`: functions hosts may call off the message thread, including their overrides | `AudioProcessor::prepareToPlay`, `AudioProcessor::releaseResources`, `AudioProcessor::numChannelsChanged`, `AudioSource::prepareToPlay`, `AudioSource::releaseResources` |
| `MessageThreadTypes`: classes that must be created and destroyed on the message thread | `Timer`, `MultiTimer`, `TimedCallback`, `AsyncUpdater`, `ChangeBroadcaster`, `Component`, `VBlankAttachment` |

## juce-nullable-dereference

| Option | Default |
| --- | --- |
| `NullableFunctions`: functions that may return `nullptr` | `AudioProcessor::getPlayHead`, `AudioProcessor::getActiveEditor`, `AudioProcessorValueTreeState::getParameter`, `AudioProcessorValueTreeState::getRawParameterValue`, `Component::getParentComponent`, `Component::findParentComponentOfClass`, `Component::getPeer`, `MessageManager::getInstanceWithoutCreating`, `AudioDeviceManager::getCurrentAudioDevice`, `Displays::getPrimaryDisplay`, `AudioProcessor::getXmlFromBinary`, `parseXML`, `parseXMLIfTagMatches`, `XmlDocument::parse`, `XmlDocument::getDocumentElement`, `ValueTree::createXml`, `WebBrowserComponent::EvaluationResult::getResult`, `WebBrowserComponent::EvaluationResult::getError`, `var::getArray`, `var::getBinaryData`, `var::getObject`, `var::getDynamicObject`, `XmlElement::getChildByName`, `XmlElement::getChildByAttribute`, `XmlElement::getChildElement`, `XmlElement::getFirstChildElement`, `XmlElement::getNextElement`, `XmlElement::getNextElementWithTagName`, `Component::getChildComponent`, `Component::getComponentAt`, `AudioProcessor::getBus`, `AudioFormatManager::createReaderFor`, `AudioFormatManager::findFormatForFileExtension` |
| `CheckDynamicCasts`: also flag dereferencing a `dynamic_cast` to a pointer | `true` |

## juce-pass-small-objects-by-value

| Option | Default |
| --- | --- |
| `Types`: class names to pass by value | `juce::Point`, `Rectangle`, `Range`, `Colour`, `Time`, `RelativeTime`, `Identifier`, `ModifierKeys`, `Justification`, `PixelRGB`, `PixelARGB`, `PixelAlpha`, `StringRef` and the `CharPointer_*` classes |

## juce-plugin-global-state

| Option | Default |
| --- | --- |
| `SingletonMacros`: macros that declare a singleton | `JUCE_DECLARE_SINGLETON;JUCE_DECLARE_SINGLETON_SINGLETHREADED;JUCE_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL;JUCE_DECLARE_SINGLETON_INLINE` |

## juce-prefer-logger

| Option | Default |
| --- | --- |
| `Streams`: streams to flag | `std::cout`, `std::cerr`, `std::clog`, `std::wcout`, `std::wcerr`, `std::wclog` |
| `Functions`: functions to flag. Those that take a `FILE*` are only flagged when it's `stdout` or `stderr` | `printf`, `vprintf`, `puts`, `putchar`, `fprintf`, `vfprintf`, `fputs`, `fputc`, `putc`, `fwrite`, `std::print`, `std::println`, `std::vprint_unicode`, `std::vprint_nonunicode` |

## juce-prefer-std-containers

| Option | Default |
| --- | --- |
| `IgnoredTypes`: JUCE types not to flag, such as `juce::CriticalSection` | none |
| `IncludeStyle`: how fixes add missing includes, `llvm` or `google` | `llvm` |

## juce-private-callback-inheritance

| Option | Default |
| --- | --- |
| `Types`: classes to inherit privately, in addition to every JUCE class whose name ends in `Listener` | `Timer`, `MultiTimer`, `HighResolutionTimer`, `AsyncUpdater`, `ChangeListener`, `ActionListener`, `ComponentListener`, `MouseListener`, `FocusChangeListener`, `AudioProcessorListener`, `MidiInputCallback`, `MidiKeyboardStateListener`, `TimeSliceClient` |

## juce-processor-state-from-editor

| Option | Default |
| --- | --- |
| `ProcessorMethods`: processor methods editors shouldn't call | `AudioProcessor::setLatencySamples`, `AudioProcessor::setPlayConfigDetails`, `AudioProcessor::setRateAndBufferSizeDetails` |

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
| `PreferJuceFunctions`: prefer JUCE's functions over their `std::` equivalents, and don't flag `juce::MathConstants` | `false` |
| `IncludeStyle`: how fixes add missing includes, `llvm` or `google` | `llvm` |

## juce-template-parameter-name

| Option | Default |
| --- | --- |
| `MinimumLength`: the shortest allowed name | `2` |

## juce-unchecked-sample-rate

| Option | Default |
| --- | --- |
| `SampleRateFunctions`: functions that return 0 until audio is prepared | `AudioProcessor::getSampleRate`, `AudioIODevice::getCurrentSampleRate` |
| `PreparedFunctions`: functions where the sample rate is always set, including their overrides | `AudioProcessor::prepareToPlay`, `AudioProcessor::processBlock`, `AudioProcessor::processBlockBypassed` |

## juce-use-after-callback

| Option | Default |
| --- | --- |
| `GuardTypes`: types that show the function checks `this` still exists | `juce::WeakReference`, `juce::Component::SafePointer`, `juce::Component::BailOutChecker`, `std::weak_ptr` |

## juce-webview-dev-server-in-release

| Option | Default |
| --- | --- |
| `DebugMacros`: macros whose `#if` branches are only built for development | `JUCE_DEBUG` |
