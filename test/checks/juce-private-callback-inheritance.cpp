#include <juce_mock.h>

class Meter
    : public juce::Component
    , public juce::Timer // EXPECT_WARNING
    , private juce::AsyncUpdater
{
public:
    void timerCallback() override
    {
    }

private:
    void handleAsyncUpdate() override
    {
    }
};

struct Watcher : juce::ValueTree::Listener // EXPECT_WARNING
{
};

class Clicker
    : public juce::Component
    , public juce::Button::Listener // EXPECT_WARNING
{
public:
    explicit Clicker(juce::Button& button)
    {
        button.addListener(this);
    }

    void buttonClicked(juce::Button*) override
    {
    }
};

class Observer : public juce::ChangeListener
{
public:
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
    }
};

void watch(juce::ChangeBroadcaster& broadcaster, Observer& observer)
{
    broadcaster.addChangeListener(&observer);
}

class Protected : protected juce::Timer
{
    void timerCallback() override
    {
    }
};
