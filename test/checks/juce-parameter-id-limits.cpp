#include <juce_mock.h>

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain 2", 0.0f, 1.0f, 0.5f)); // EXPECT_WARNING

    layout.add(std::make_unique<juce::AudioParameterBool>("thisParameterIdentifierIsMuchTooLong", "Long", false)); // EXPECT_WARNING

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "bypass", 1 }, "Bypass", false));

    return layout;
}

juce::AudioProcessorValueTreeState::ParameterLayout createOtherLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain", 0.0f, 1.0f, 0.5f));
    return layout;
}

class Processor : public juce::AudioProcessor
{
public:
    Processor()
    {
        addParameter(new juce::AudioParameterFloat({ "mix", 1 }, "Mix", 0.0f, 1.0f, 1.0f));
        addParameter(new juce::AudioParameterFloat({ "mix", 1 }, "Mix", 0.0f, 1.0f, 1.0f)); // EXPECT_WARNING
    }
};
