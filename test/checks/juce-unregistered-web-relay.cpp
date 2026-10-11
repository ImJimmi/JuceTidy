#include <juce_mock.h>

juce::WebBrowserComponent::Options withMode(const juce::WebBrowserComponent::Options& options, juce::WebComboBoxRelay& relay);

class Editor : public juce::AudioProcessorEditor
{
public:
    Editor(juce::AudioProcessor& p, juce::RangedAudioParameter& gain, juce::RangedAudioParameter& mute)
        : juce::AudioProcessorEditor(p)
        , gainAttachment(gain, gainRelay)
        , muteAttachment(mute, muteRelay)
    {
        addAndMakeVisible(webview);
        ownedRelay->setValue(0.5f);
    }

    void resized() override
    {
        webview.setBounds(getLocalBounds());
    }

private:
    juce::WebSliderRelay gainRelay{ "gain" };
    juce::WebToggleButtonRelay muteRelay{ "mute" }; // EXPECT_WARNING
    juce::WebComboBoxRelay modeRelay{ "mode" };
    std::unique_ptr<juce::WebSliderRelay> ownedRelay = std::make_unique<juce::WebSliderRelay>("owned");
    std::unique_ptr<juce::WebSliderRelay> forgottenRelay = std::make_unique<juce::WebSliderRelay>("forgotten"); // EXPECT_WARNING
    juce::WebControlParameterIndexReceiver indexReceiver; // EXPECT_WARNING
    juce::WebBrowserComponent webview{ withMode(juce::WebBrowserComponent::Options{}
                                                    .withNativeIntegrationEnabled()
                                                    .withOptionsFrom(gainRelay)
                                                    .withOptionsFrom(*ownedRelay),
                                                modeRelay) };
    juce::WebSliderParameterAttachment gainAttachment;
    juce::WebToggleButtonParameterAttachment muteAttachment;
};

class DefinedElsewhere : public juce::Component
{
public:
    DefinedElsewhere();

private:
    juce::WebSliderRelay relay{ "gain" };
};
