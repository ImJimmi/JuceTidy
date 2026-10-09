#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        const auto xml = apvts.state.createXml(); // EXPECT_WARNING
        copyXmlToBinary(*xml, destData);

        const auto copy = apvts.copyState();
        (void) copy;
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        const auto xml = getXmlFromBinary(data, sizeInBytes);

        if (xml != nullptr && xml->hasTagName("state"))
        {
            apvts.state = juce::ValueTree::fromXml(*xml); // EXPECT_WARNING
            apvts.state.setProperty("version", 1, nullptr); // EXPECT_WARNING

            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            (void) apvts.state.getProperty("version");
        }
    }

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "state", {} };
};
