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

void evaluate(juce::WebBrowserComponent& browser)
{
    browser.evaluateJavascript("1 + 1", [](juce::WebBrowserComponent::EvaluationResult result) {
        const auto value = *result.getResult(); // EXPECT_WARNING
        (void) value;

        if (const auto* error = result.getError())
            (void) error->message;

        (void) result.getError()->message; // EXPECT_WARNING
    });
}

void variants(const juce::var& value, const juce::XmlElement& xml, juce::Component& component, juce::AudioProcessor& processor)
{
    (void) value.getArray()->size(); // EXPECT_WARNING
    (void) *value.getArray(); // EXPECT_WARNING
    value.getDynamicObject()->setProperty("x", 1); // EXPECT_WARNING

    if (auto* array = value.getArray())
        (void) array->size();

    if (value.isArray())
        (void) value.getArray()->size();
    else if (value.isObject())
        (void) value.getObject();
    else if (value.isBinaryData())
        (void) value.getArray()->size(); // EXPECT_WARNING

    if (value.isArray())
    {
        const auto& elements = *value.getArray();
        (void) elements;
    }

    (void) xml.getChildByName("child")->getTagName(); // EXPECT_WARNING
    component.getChildComponent(0)->repaint(); // EXPECT_WARNING
    (void) processor.getBus(true, 0)->getNumberOfChannels(); // EXPECT_WARNING

    (void) dynamic_cast<juce::Slider*>(&component)->getValue(); // EXPECT_WARNING
    (void) dynamic_cast<juce::Slider&>(component).getValue();

    if (auto* slider = dynamic_cast<juce::Slider*>(&component))
        (void) slider->getValue();
}
