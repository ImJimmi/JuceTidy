#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    Processor()
    {
        samplesPerMillisecond = getSampleRate() / 1000.0;
        millisecondsPerSample = 1000.0 / getSampleRate(); // EXPECT_WARNING
    }

    void prepareToPlay(double, int) override
    {
        period = 1.0 / getSampleRate();
    }

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        phase += frequency / getSampleRate();
    }

private:
    double samplesPerMillisecond = 0.0;
    double millisecondsPerSample = 0.0;
    double period = 0.0;
    double phase = 0.0;
    double frequency = 440.0;
};

class Editor : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor& p)
        : juce::AudioProcessorEditor{ p }
    {
        const auto sampleRate = p.getSampleRate();
        latency = 512.0 / sampleRate; // EXPECT_WARNING

        const auto checkedSampleRate = p.getSampleRate();

        if (checkedSampleRate > 0.0)
            latency = 512.0 / checkedSampleRate;

        latency = 512.0f / static_cast<float>(p.getSampleRate()); // EXPECT_WARNING
        latency /= p.getSampleRate(); // EXPECT_WARNING
    }

private:
    double latency = 0.0;
};
