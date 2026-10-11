#include <juce_mock.h>

class Editor : public juce::AudioProcessorEditor
{
public:
    explicit Editor(juce::AudioProcessor& p)
        : juce::AudioProcessorEditor(p)
    {
        addAndMakeVisible(webview);
        webview.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        webview.emitEventIfBrowserIsVisible("ready", 1); // EXPECT_WARNING
        owned->evaluateJavascript("initialise()"); // EXPECT_WARNING

        juce::MessageManager::callAsync([this] { webview.emitEventIfBrowserIsVisible("later", 1); });
    }

    void stateChanged()
    {
        webview.emitEventIfBrowserIsVisible("state", 1);
    }

private:
    juce::WebBrowserComponent webview;
    std::unique_ptr<juce::WebBrowserComponent> owned = std::make_unique<juce::WebBrowserComponent>();
};

class Browser : public juce::WebBrowserComponent
{
public:
    Browser()
    {
        emitEventIfBrowserIsVisible("constructed", 1); // EXPECT_WARNING
    }
};

class Panel
{
public:
    explicit Panel(juce::WebBrowserComponent& browserIn)
        : browser(browserIn)
    {
        browser.emitEventIfBrowserIsVisible("panel-created", 1);
        browserIn.evaluateJavascript("showPanel()");
    }

private:
    juce::WebBrowserComponent& browser;
};
