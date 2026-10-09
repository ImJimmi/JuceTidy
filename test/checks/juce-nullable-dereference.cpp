#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        const auto bpm = getPlayHead()->getPosition().getBpm(); // EXPECT_WARNING
        (void) bpm;

        if (auto* playHead = getPlayHead())
            (void) playHead->getPosition();

        const auto gain = apvts->getRawParameterValue("gain")->load(); // EXPECT_WARNING
        (void) gain;

        const auto& parameter = *apvts->getParameter("gain"); // EXPECT_WARNING
        (void) parameter;
    }

private:
    juce::AudioProcessorValueTreeState* apvts = nullptr;
};

void component(juce::Component& component)
{
    component.getParentComponent()->repaint(); // EXPECT_WARNING

    if (auto* parent = component.getParentComponent())
        parent->repaint();
}

void restoreState(const void* data, int size)
{
    const auto tree = juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(data, size)); // EXPECT_WARNING
    const auto name = juce::parseXML("<a/>")->getTagName(); // EXPECT_WARNING

    if (const auto xml = juce::AudioProcessor::getXmlFromBinary(data, size))
        (void) xml->getTagName();

    const auto type = tree.createXml()->getTagName(); // EXPECT_WARNING
    (void) type;

    (void) tree;
    (void) name;
}
