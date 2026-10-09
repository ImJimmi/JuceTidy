#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void prepareToPlay(double, int samplesPerBlock) override
    {
        blockSize = samplesPerBlock;
        maxSamples = static_cast<size_t>(samplesPerBlock);
        scratch.setSize(2, samplesPerBlock);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        for (int i = 0; i < blockSize; ++i) // EXPECT_WARNING
            buffer.getWritePointer(0)[i] = 0.0f;

        scratch.makeCopyOf(buffer, true);
        scratch.setSize(2, blockSize, false, false, true); // EXPECT_WARNING

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.getWritePointer(0)[i] = 0.0f;

        jassert(static_cast<size_t>(buffer.getNumSamples()) <= maxSamples);
    }

    void releaseResources() override {}

private:
    int blockSize = 0;
    size_t maxSamples = 0;
    juce::AudioBuffer<float> scratch;
};
