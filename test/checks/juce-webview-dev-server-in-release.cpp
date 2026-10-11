#include <juce_mock.h>

using Options = juce::WebBrowserComponent::Options;

std::optional<juce::WebBrowserComponent::Resource> provide(const juce::String& url);

static constexpr auto devServer = "http://localhost:5173/";

#if JUCE_DEBUG
static constexpr auto debugServer = "http://localhost:5173/";
#endif

class Browser : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;

    bool pageAboutToLoad(const juce::String& url) override
    {
        return url == getResourceProviderRoot()
            || url == devServer // EXPECT_WARNING
#if JUCE_DEBUG
            || url == debugServer
            || url == "http://127.0.0.1:5173/"
#endif
            || url == "http://localhost:8080/"; // EXPECT_WARNING
    }
};

class Editor : public juce::Component
{
public:
    Editor()
    {
#if JUCE_DEBUG && JUCE_MAC
        webview.goToURL("http://localhost:5173/");
#endif
        webview.goToURL(devServer); // EXPECT_WARNING
        webview.goToURL("https://juce.com/");
        webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    }

private:
    Browser webview{ Options{}
                         .withResourceProvider(provide, juce::URL{ "http://localhost:5173/" }.getOrigin()) // EXPECT_WARNING
                         .withResourceProvider(provide)
#ifdef JUCE_DEBUG
                         .withResourceProvider(provide, juce::URL{ "http://localhost:5173/" }.getOrigin())
#endif
    };
};
