#include <juce_mock.h>

using Options = juce::WebBrowserComponent::Options;

std::optional<juce::WebBrowserComponent::Resource> provide(const juce::String& url);

const auto userDataFolder = juce::File::getSpecialLocation(juce::File::tempDirectory);

class MissingBackend : public juce::Component
{
    juce::WebBrowserComponent webview{ Options{}.withNativeIntegrationEnabled() }; // EXPECT_WARNING
    juce::WebBrowserComponent explicitDefault{ Options{}
                                                   .withBackend(Options::Backend::defaultBackend)
                                                   .withResourceProvider(provide) }; // EXPECT_WARNING
};

class MissingUserDataFolder : public juce::Component
{
    juce::WebBrowserComponent webview{ Options{}
                                           .withBackend(Options::Backend::webview2) // EXPECT_WARNING
                                           .withNativeIntegrationEnabled() };
    juce::WebBrowserComponent otherOptions{ Options{}
                                                .withBackend(Options::Backend::webview2) // EXPECT_WARNING
                                                .withWinWebView2Options(Options::WinWebView2{}.withStatusBarDisabled()) };
};

class NothingNeedingWebView2 : public juce::Component
{
    juce::WebBrowserComponent webview{ Options{}.withUserAgent("JUCE") };
    juce::WebBrowserComponent disabled{ Options{}.withNativeIntegrationEnabled(false) };
    juce::WebBrowserComponent plain;
};

class Configured : public juce::Component
{
    juce::WebBrowserComponent webview{ Options{}
                                           .withBackend(Options::Backend::webview2)
                                           .withWinWebView2Options(Options::WinWebView2{}.withUserDataFolder(userDataFolder))
                                           .withNativeIntegrationEnabled() };
};

static Options buildOptions(const std::vector<juce::String>& events)
{
    auto options = Options{}
                       .withBackend(Options::Backend::webview2)
                       .withWinWebView2Options(Options::WinWebView2{}.withUserDataFolder(userDataFolder))
                       .withNativeIntegrationEnabled();

    for (const auto& event : events)
        options = options.withEventListener(event, [](juce::var) {});

    return options;
}

class Browser : public juce::WebBrowserComponent
{
public:
    explicit Browser(const Options& options)
        : juce::WebBrowserComponent{ options }
    {
    }
};

class BuiltInHelper : public juce::Component
{
public:
    BuiltInHelper()
        : webview{ std::make_unique<Browser>(buildOptions({ "ready" }).withResourceProvider(provide)) }
    {
    }

private:
    std::unique_ptr<Browser> webview;
};

static Options withoutBackend()
{
    auto options = Options{}.withUserScript("console.log('hello')"); // EXPECT_WARNING
    return options;
}

class FromHelper : public juce::Component
{
    std::unique_ptr<juce::WebBrowserComponent> webview = std::make_unique<juce::WebBrowserComponent>(withoutBackend());
};

Options addIntegration(const Options& options)
{
    return options.withNativeIntegrationEnabled();
}

void configure(Options& options);

class Unknown : public juce::Component
{
public:
    Unknown()
    {
        auto options = Options{}.withNativeIntegrationEnabled();
        configure(options);
        owned = std::make_unique<juce::WebBrowserComponent>(options);
    }

private:
    juce::WebBrowserComponent webview{ addIntegration(Options{}) };
    std::unique_ptr<juce::WebBrowserComponent> owned;
};
