#include <juce_mock.h>

class Meter : public juce::Component
    , private juce::Timer
{
public:
    Meter()
    {
        startTimerHz(60);
    }

private:
    void timerCallback() override // EXPECT_WARNING
    {
        repaint();
    }
};

class FastMillisecondMeter : public juce::Component
    , private juce::Timer
{
public:
    FastMillisecondMeter()
    {
        startTimer(16);
    }

private:
    void timerCallback() override;
};

void FastMillisecondMeter::timerCallback() // EXPECT_WARNING
{
    repaint();
}

class Clock : public juce::Component
    , private juce::Timer
{
public:
    Clock()
    {
        startTimer(1000);
    }

private:
    void timerCallback() override
    {
        repaint();
    }
};

class PollingMeter : public juce::Component
    , private juce::Timer
{
public:
    PollingMeter()
    {
        startTimerHz(60);
    }

private:
    void timerCallback() override
    {
        if (needsRepaint.exchange(false))
            repaint();
    }

    std::atomic<bool> needsRepaint{ false };
};

class UnknownRate : public juce::Component
    , private juce::Timer
{
public:
    explicit UnknownRate(int hz)
    {
        startTimerHz(hz);
    }

private:
    void timerCallback() override
    {
        repaint();
    }
};

class NotAComponent : private juce::Timer
{
public:
    NotAComponent()
    {
        startTimerHz(60);
    }

private:
    void timerCallback() override
    {
        component.repaint();
    }

    juce::Component component;
};

class SyncedMeter : public juce::Component
{
private:
    juce::VBlankAttachment vBlank{ this, [this] { repaint(); } };
};
