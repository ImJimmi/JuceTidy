#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    explicit Editor(juce::Slider& externalSlider)
    {
        int clicks = 0;
        juce::String title = "Title";

        button.onClick = [&] { ++clicks; }; // EXPECT_WARNING

        slider.onValueChange = [&title, this] { (void) title; repaint(); }; // EXPECT_WARNING

        button.onClick = [clicks, this] { (void) clicks; repaint(); };
        slider.onValueChange = [&externalSlider] { (void) externalSlider.getValue(); };
        button.onClick = [this] { ++count; };

        static int shared = 0;
        button.onClick = [&] { ++shared; };

        auto immediate = [&] { ++clicks; };
        immediate();
    }

private:
    juce::TextButton button;
    juce::Slider slider;
    int count = 0;
};

juce::WebBrowserComponent::Options createOptions()
{
    int counter = 0;
    juce::String greeting = "Hello";

    return juce::WebBrowserComponent::Options{}
        .withNativeFunction("count", [&](auto&, auto complete) { complete(++counter); }) // EXPECT_WARNING
        .withEventListener("greet", [&greeting](juce::var) { (void) greeting; }) // EXPECT_WARNING
        .withResourceProvider([&](const juce::String&) { (void) greeting; return std::optional<juce::WebBrowserComponent::Resource>{}; }) // EXPECT_WARNING
        .withNativeFunction("copy", [counter](auto&, auto complete) { complete(counter); });
}

void evaluate(juce::WebBrowserComponent& browser)
{
    int answer = 0;
    browser.evaluateJavascript("6 * 7", [&answer](juce::WebBrowserComponent::EvaluationResult) { answer = 42; }); // EXPECT_WARNING
}
