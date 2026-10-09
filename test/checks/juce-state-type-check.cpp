#include <juce_mock.h>

class Unchecked : public juce::AudioProcessor
{
public:
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            apvts.replaceState(juce::ValueTree::fromXml(*xml)); // EXPECT_WARNING
    }

    juce::AudioProcessorValueTreeState apvts;
};

class Assigned : public juce::AudioProcessor
{
public:
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
};

void Assigned::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.state = juce::ValueTree::fromXml(*xml); // EXPECT_WARNING
}

class Checked : public juce::AudioProcessor
{
public:
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary(data, sizeInBytes); xml != nullptr && xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    juce::AudioProcessorValueTreeState apvts;
};

class Nested : public juce::AudioProcessor
{
public:
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            if (auto tree = juce::ValueTree::fromXml(*xml).getChildWithName("Parameters"); tree.isValid())
                apvts.replaceState(tree);
    }

    juce::AudioProcessorValueTreeState apvts;
};

void restore(juce::AudioProcessorValueTreeState& apvts, const juce::ValueTree& tree)
{
    apvts.replaceState(tree);
}
