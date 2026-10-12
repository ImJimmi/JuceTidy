#include <juce_mock.h>

class Meter
    : public juce::Component
    , private juce::Timer // EXPECT_WARNING
{
public:
    Meter()
    {
        startTimerHz(60);
    }

private:
    void timerCallback() override
    {
    }
};

class Display : public juce::Component
{
public:
    Display()
    {
        callback.startTimerHz(30);
    }

private:
    juce::TimedCallback callback{ [this] {
        repaint();
    } };
};
