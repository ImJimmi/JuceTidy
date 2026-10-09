#include <juce_mock.h>

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 0.5f)); // EXPECT_WARNING

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "bypass" }, "Bypass", false)); // EXPECT_WARNING

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "mute", 0 }, "Mute", false)); // EXPECT_WARNING

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "mix", 1 }, "Mix", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("solo", 2), "Solo", false));

    const juce::ParameterID panId{ "pan", 1 };
    layout.add(std::make_unique<juce::AudioParameterFloat>(panId, "Pan", -1.0f, 1.0f, 0.0f));

    return layout;
}

class Processor : public juce::AudioProcessor
{
public:
    Processor()
    {
        addParameter(new juce::AudioParameterFloat("drive", "Drive", 0.0f, 1.0f, 0.0f)); // EXPECT_WARNING

        addParameter(new juce::AudioParameterFloat({ "tone", 1 }, "Tone", 0.0f, 1.0f, 0.0f));
    }
};

struct CustomParameter : public juce::AudioParameterFloat
{
    CustomParameter()
        : juce::AudioParameterFloat("custom", "Custom", 0.0f, 1.0f, 0.0f) // EXPECT_WARNING
    {
    }
};
