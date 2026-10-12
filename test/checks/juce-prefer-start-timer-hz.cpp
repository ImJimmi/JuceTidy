#include <juce_mock.h>

class Meter : private juce::Timer
{
public:
    Meter()
    {
        startTimer(1000 / 60); // EXPECT_WARNING
        startTimer(16);
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
        callback.startTimer(1000 / 30); // EXPECT_WARNING
        callback.startTimerHz(30);
    }

private:
    juce::TimedCallback callback{ [this] {
        repaint();
    } };
};
