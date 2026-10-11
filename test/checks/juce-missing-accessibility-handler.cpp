#include <juce_mock.h>

class XYPad : public juce::Component // EXPECT_WARNING
{
public:
    void mouseDown(const juce::MouseEvent&) override
    {
    }

    void mouseDrag(const juce::MouseEvent&) override
    {
    }
};

class ClickableArea : public juce::Component // EXPECT_WARNING
{
public:
    void mouseUp(const juce::MouseEvent&) override;
};

class AccessibleXYPad : public juce::Component
{
public:
    void mouseDown(const juce::MouseEvent&) override
    {
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
};

class DerivedAccessibleXYPad : public AccessibleXYPad
{
public:
    void mouseDrag(const juce::MouseEvent&) override
    {
    }
};

class CustomSlider : public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent&) override
    {
    }
};

class HoverHighlight : public juce::Component
{
public:
    void mouseEnter(const juce::MouseEvent&) override
    {
    }
};

class Decoration : public juce::Component
{
public:
    Decoration()
    {
        setAccessible(false);
    }

    void mouseDown(const juce::MouseEvent&) override
    {
    }
};

class Editor : public juce::AudioProcessorEditor
{
public:
    void mouseDown(const juce::MouseEvent&) override
    {
    }
};
