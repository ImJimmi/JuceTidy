#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto* data = buffer.getWritePointer(0);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            data[sample] *= gain.load(); // EXPECT_WARNING
            data[sample] *= gain; // EXPECT_WARNING
            data[sample] *= cutoff->load(); // EXPECT_WARNING
            data[sample] *= *cutoff; // EXPECT_WARNING
            data[sample] *= mix->get(); // EXPECT_WARNING
            data[sample] *= *mix; // EXPECT_WARNING
            data[sample] *= voiceGains[sample % 4].load();
        }

        const auto currentGain = gain.load();

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            data[sample] *= currentGain;

        for (auto& voiceGain : voiceGains)
            voiceGain.store(voiceGain.load() * 0.5f);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto apply = [this](float value) { return value * gain.load(); };
            data[sample] = apply(data[sample]);
        }
    }

    void updateMeters()
    {
        for (int i = 0; i < 4; ++i)
            meterValues[i] = gain.load();
    }

private:
    std::atomic<float> gain{ 1.0f };
    std::atomic<float>* cutoff = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
    std::atomic<float> voiceGains[4];
    float meterValues[4]{};
};
