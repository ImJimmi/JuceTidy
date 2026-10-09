#include <juce_mock.h>

class WithLayoutMethod : public juce::Component
{
public:
    WithLayoutMethod()
    {
        resized(); // EXPECT_WARNING
    }

    void resized() override
    {
        updateLayout();
    }

private:
    void updateLayout()
    {
        child.resized(); // EXPECT_WARNING
    }

    juce::Slider child;
};

class WithoutLayoutMethod : public juce::Slider
{
public:
    void setRange()
    {
        resized(); // EXPECT_WARNING
    }

    void resized() override
    {
        juce::Slider::resized();
    }
};

class ChainsToBaseLayout : public WithoutLayoutMethod
{
public:
    void resized() override
    {
        updateLayout();
    }

private:
    void updateLayout()
    {
        WithoutLayoutMethod::resized();
    }

    void somewhereElse()
    {
        WithoutLayoutMethod::resized(); // EXPECT_WARNING
    }
};
