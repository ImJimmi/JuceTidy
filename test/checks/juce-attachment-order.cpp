#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "state", {} };
};

class WrongOrder : public juce::AudioProcessorEditor
{
public:
    explicit WrongOrder(Processor& p)
        : juce::AudioProcessorEditor(p)
        , gainAttachment(p.apvts, "gain", gainSlider)
    {
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "bypass", bypassButton);
        modeAttachment.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(p.apvts, "mode", *modeBox));
    }

private:
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment; // EXPECT_WARNING
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment; // EXPECT_WARNING
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment; // EXPECT_WARNING
    juce::Slider gainSlider;
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::ComboBox> modeBox;
};

class InClassInitialiser : public juce::Component
{
public:
    explicit InClassInitialiser(Processor& p)
        : processor(p)
    {
    }

private:
    Processor& processor;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment{ processor.apvts, "gain", slider }; // EXPECT_WARNING
    juce::Slider slider;
};

class CorrectOrder : public juce::AudioProcessorEditor
{
public:
    explicit CorrectOrder(Processor& p)
        : juce::AudioProcessorEditor(p)
        , gainAttachment(p.apvts, "gain", gainSlider)
    {
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "bypass", bypassButton);
    }

private:
    juce::Slider gainSlider;
    juce::ToggleButton bypassButton;
    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
};

void attachmentToExternalSlider(Processor& p, juce::Slider& slider)
{
    juce::AudioProcessorValueTreeState::SliderAttachment attachment(p.apvts, "gain", slider);
}
