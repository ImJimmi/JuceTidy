#include <juce_mock.h>

void process(juce::AudioBuffer<float>& buffer, float gain)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            data[sample] *= gain;
    }

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        buffer.getWritePointer(0)[sample] *= gain; // EXPECT_WARNING

        buffer.setSample(1, sample, buffer.getSample(0, sample)); // EXPECT_WARNING
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.setSample(channel, 0, 0.0f);

    buffer.setSample(0, 0, buffer.getSample(0, 0));
}
