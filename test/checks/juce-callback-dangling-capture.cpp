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
