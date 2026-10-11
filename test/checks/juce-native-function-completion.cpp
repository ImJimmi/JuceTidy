#include <juce_mock.h>

void save(const juce::var& value);

const auto options = juce::WebBrowserComponent::Options{}
                         .withNativeFunction("save", [](auto& args, auto) { save(args[0]); }) // EXPECT_WARNING
                         .withNativeFunction("load", [](auto&, auto complete) {}) // EXPECT_WARNING
                         .withNativeFunction("greet", [](auto& args, auto complete) { complete(args[0]); })
                         .withNativeFunction("later", [](auto&, auto complete) { juce::MessageManager::callAsync([complete] { complete("done"); }); })
                         .withNativeFunction("typed", [](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion complete) { complete(1); })
                         .withNativeFunction("forgotten", [](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion) {}); // EXPECT_WARNING
