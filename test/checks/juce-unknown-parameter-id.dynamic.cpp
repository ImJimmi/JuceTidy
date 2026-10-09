#include <juce_mock.h>

juce::AudioProcessorValueTreeState::ParameterLayout createLayout(const juce::String& bandId)
{
    return { std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain", 0.0f, 1.0f, 0.5f),
             std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ bandId, 1 }, "Band", 0.0f, 1.0f, 0.5f) };
}

void connect(juce::AudioProcessorValueTreeState& apvts)
{
    apvts.getRawParameterValue("band1");
}
