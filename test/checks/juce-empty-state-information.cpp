#include <juce_mock.h>

class EmptyProcessor : public juce::AudioProcessor
{
public:
    void getStateInformation(juce::MemoryBlock&) override // EXPECT_WARNING
    {
    }

    void setStateInformation(const void* data, int size) override // EXPECT_WARNING
    {
        juce::ignoreUnused(data, size);
    }
};

class HalfProcessor : public juce::AudioProcessor
{
public:
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        copyXmlToBinary(*state.createXml(), destData);
    }

    void setStateInformation(const void* data, int size) override;

private:
    juce::ValueTree state;
};

void HalfProcessor::setStateInformation(const void* data, int size) // EXPECT_WARNING
{
    (void) data;
    (void) size;
}

class SavingProcessor : public juce::AudioProcessor
{
public:
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        copyXmlToBinary(*state.createXml(), destData);
    }

    void setStateInformation(const void* data, int size) override
    {
        if (const auto xml = getXmlFromBinary(data, size))
            state = juce::ValueTree::fromXml(*xml);
    }

private:
    juce::ValueTree state;
};
