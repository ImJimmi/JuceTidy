#include <juce_mock.h>

#include <atomic>

class Processor : public juce::AudioProcessor,
                  private juce::AudioProcessorValueTreeState::Listener,
                  private juce::AudioProcessorParameter::Listener
{
private:
    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        if (parameterID == "gain")
            gain.store(newValue);

        if (parameterID.toLowerCase() == "mix") // EXPECT_WARNING
            mix.store(newValue);

        if (parameterID == juce::String("pan")) // EXPECT_WARNING
            pan.store(newValue);

        const auto key = parameterID + "_smoothed"; // EXPECT_WARNING

        const auto standard = parameterID.toStdString(); // EXPECT_WARNING

        updateLabel(parameterID);
        (void) key;
        (void) standard;
    }

    void updateLabel(const juce::String& parameterID)
    {
        label = parameterID.substring(1); // EXPECT_WARNING
    }

    void parameterValueChanged(int, float) override
    {
        const juce::ScopedLock lock(criticalSection); // EXPECT_WARNING
    }

    void parameterGestureChanged(int, bool) override
    {
        juce::Logger::writeToLog("gesture"); // EXPECT_WARNING
    }

    juce::CriticalSection criticalSection;
    juce::String label;
    std::atomic<float> gain{ 0.0f };
    std::atomic<float> mix{ 0.0f };
    std::atomic<float> pan{ 0.0f };
};

class SmoothedParameter : public juce::AudioParameterFloat
{
public:
    using juce::AudioParameterFloat::AudioParameterFloat;

protected:
    void valueChanged(float newValue) override
    {
        history.push_back(newValue); // EXPECT_WARNING
    }

private:
    std::vector<float> history;
};

class HostListener : public juce::AudioProcessorListener
{
public:
    void audioProcessorParameterChanged(juce::AudioProcessor*, int, float newValue) override
    {
        latest.store(newValue);
    }

private:
    std::atomic<float> latest{ 0.0f };
};
