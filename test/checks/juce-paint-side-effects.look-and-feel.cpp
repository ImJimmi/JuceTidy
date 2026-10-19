#include <juce_mock.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics&, juce::Button& button, const juce::Colour&, bool, bool) override
    {
        button.repaint(); // EXPECT_WARNING
        addShadow(button);
    }

    int getTextButtonWidthToFitText(juce::TextButton& button, int) override
    {
        button.setBounds(0, 0, 10, 10);
        return 10;
    }

private:
    void addShadow(juce::Button& button)
    {
        if (auto* parent = button.getParentComponent())
            parent->addChildComponent(shadow, 0); // EXPECT_WARNING
    }

    juce::Component shadow;
};

class RotaryStyle : public juce::Slider::LookAndFeelMethods
{
public:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider& slider) override
    {
        slider.setVisible(true); // EXPECT_WARNING
    }
};

class Painter
{
public:
    void draw(juce::Graphics&, juce::Component& component)
    {
        component.setBounds(0, 0, 10, 10);
    }
};
