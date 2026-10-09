#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        const juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
        preparedGain.prepare(spec);
        smoothed.reset(sampleRate, 0.05);
        envelope.setSampleRate(sampleRate);
        oversampling.initProcessing(static_cast<size_t>(samplesPerBlock));
        prepareSharedFilter(sharedFilter, spec);
    }

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        smoothed.setTargetValue(unpreparedGain.processSample(0.0f));
    }

    void releaseResources() override {}

private:
    static void prepareSharedFilter(juce::dsp::IIR::Filter<float>& filter, const juce::dsp::ProcessSpec& spec)
    {
        filter.prepare(spec);
    }

    juce::dsp::Gain<float> preparedGain;
    juce::dsp::Gain<float> unpreparedGain; // EXPECT_WARNING
    juce::SmoothedValue<float> smoothed;
    juce::SmoothedValue<float> unsmoothed; // EXPECT_WARNING
    juce::ADSR envelope;
    juce::Reverb reverb; // EXPECT_WARNING
    juce::dsp::Oversampling<float> oversampling;
    juce::dsp::IIR::Filter<float> sharedFilter;
    juce::dsp::IIR::Filter<float> unpreparedFilter; // EXPECT_WARNING
};

class DefinedElsewhere
{
public:
    void prepare(double sampleRate);

private:
    juce::SmoothedValue<float> smoothed;
};
