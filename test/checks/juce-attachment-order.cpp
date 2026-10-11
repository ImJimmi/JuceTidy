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

class WebEditor : public juce::AudioProcessorEditor
{
public:
    WebEditor(juce::AudioProcessor& p, juce::RangedAudioParameter& gain, juce::RangedAudioParameter& bypass)
        : juce::AudioProcessorEditor(p)
        , gainAttachment(gain, gainRelay)
        , bypassAttachment(bypass, bypassRelay)
    {
    }

private:
    juce::WebSliderParameterAttachment gainAttachment; // EXPECT_WARNING
    juce::WebSliderRelay gainRelay{ "gain" };
    juce::WebBrowserComponent webview{ juce::WebBrowserComponent::Options{}
                                           .withOptionsFrom(gainRelay)
                                           .withOptionsFrom(bypassRelay)
                                           .withOptionsFrom(indexReceiver) };
    juce::WebToggleButtonRelay bypassRelay{ "bypass" }; // EXPECT_WARNING
    juce::WebControlParameterIndexReceiver indexReceiver; // EXPECT_WARNING
    juce::WebToggleButtonParameterAttachment bypassAttachment;
};

class OwnedBrowser : public juce::Component
{
public:
    OwnedBrowser()
    {
        browser = std::make_unique<juce::WebBrowserComponent>(juce::WebBrowserComponent::Options{}.withOptionsFrom(relay));
    }

private:
    std::unique_ptr<juce::WebBrowserComponent> browser;
    juce::WebComboBoxRelay relay{ "mode" }; // EXPECT_WARNING
};

class CorrectWebOrder : public juce::AudioProcessorEditor
{
public:
    CorrectWebOrder(juce::AudioProcessor& p, juce::RangedAudioParameter& gain)
        : juce::AudioProcessorEditor(p)
        , gainAttachment(gain, gainRelay)
    {
        owned = std::make_unique<juce::WebBrowserComponent>(juce::WebBrowserComponent::Options{}.withOptionsFrom(modeRelay));
    }

private:
    juce::WebSliderRelay gainRelay{ "gain" };
    juce::WebComboBoxRelay modeRelay{ "mode" };
    juce::WebBrowserComponent webview{ juce::WebBrowserComponent::Options{}.withOptionsFrom(gainRelay) };
    std::unique_ptr<juce::WebBrowserComponent> owned;
    juce::WebSliderParameterAttachment gainAttachment;
};
