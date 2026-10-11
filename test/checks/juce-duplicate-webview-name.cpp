#include <juce_mock.h>

using Options = juce::WebBrowserComponent::Options;

class Editor : public juce::Component
{
    juce::WebSliderRelay gainRelay{ "gain" };
    juce::WebSliderRelay outputRelay{ "gain" }; // EXPECT_WARNING
    juce::WebToggleButtonRelay gainToggle{ "gain" };
    juce::WebBrowserComponent webview{ Options{}
                                           .withNativeIntegrationEnabled()
                                           .withOptionsFrom(gainRelay)
                                           .withOptionsFrom(outputRelay)
                                           .withOptionsFrom(gainToggle)
                                           .withNativeFunction("save", [](auto&, auto complete) { complete(1); })
                                           .withNativeFunction("load", [](auto&, auto complete) { complete(1); })
                                           .withNativeFunction("save", [](auto&, auto complete) { complete(2); }) }; // EXPECT_WARNING
};

static Options withNativeFunctions()
{
    auto options = Options{}.withNativeFunction("reset", [](auto&, auto complete) { complete(1); });
    options = options.withNativeFunction("reset", [](auto&, auto complete) { complete(1); }); // EXPECT_WARNING
    return options;
}

class InitialiserList : public juce::Component
{
public:
    InitialiserList()
        : first("mix")
        , second("mix") // EXPECT_WARNING
        , webview{ withNativeFunctions().withOptionsFrom(first).withOptionsFrom(second) }
    {
    }

private:
    juce::WebSliderRelay first;
    juce::WebSliderRelay second;
    juce::WebSliderRelay gainRelay{ "gain" };
    juce::WebBrowserComponent webview;
};
