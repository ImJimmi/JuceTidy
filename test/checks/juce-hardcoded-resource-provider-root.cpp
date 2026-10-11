#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    Editor()
    {
        webview.goToURL("juce://juce.backend/"); // EXPECT_WARNING
        webview.goToURL("https://juce.backend/index.html"); // EXPECT_WARNING
        webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot() + "index.html");
        webview.goToURL("https://juce.com/");
        webview.goToURL("https://juce.backend.example.com/");
    }

    void fetchData()
    {
        webview.evaluateJavascript(R"(
            const response = await fetch("/data.json");
            const fallback = await fetch("juce://juce.backend/data.json"); // EXPECT_WARNING
        )");
    }

private:
    juce::WebBrowserComponent webview;
};

static constexpr auto backendRoot = "juce://juce.backend"; // EXPECT_WARNING

class Browser : public juce::WebBrowserComponent
{
public:
    bool pageAboutToLoad(const juce::String& url) override
    {
        return url == "juce://juce.backend/" // EXPECT_WARNING
            || url.startsWith("https://juce.backend/assets/"); // EXPECT_WARNING
    }
};

void configure(juce::WebBrowserComponent& browser)
{
    const juce::String index = "juce://juce.backend/index.html"; // EXPECT_WARNING
    const juce::URL api{ "https://juce.backend/api" }; // EXPECT_WARNING
    const juce::StringRef root = "juce://juce.backend/"; // EXPECT_WARNING
    const juce::StringRef assets = "juce://juce.backend/assets/"; // EXPECT_WARNING
    const char* raw = "juce://juce.backend/"; // EXPECT_WARNING
    browser.emitEventIfBrowserIsVisible("root", "juce://juce.backend"); // EXPECT_WARNING
    browser.evaluateJavascript("fetch('juce://juce.backend/data.json')"); // EXPECT_WARNING
    (void) index;
    (void) api;
    (void) root;
    (void) assets;
    (void) raw;
}
