#include <juce_mock.h>

bool compare(const juce::ValueTree& state, const juce::XmlElement& xml, const juce::ValueTree& other)
{
    if (state == juce::ValueTree::fromXml(xml)) // EXPECT_WARNING
        return true;

    if (other.createCopy() != state) // EXPECT_WARNING
        return false;

    if (state == juce::ValueTree(juce::Identifier("Settings"))) // EXPECT_WARNING
        return true;

    return state == other || state.isEquivalentTo(other.createCopy());
}
