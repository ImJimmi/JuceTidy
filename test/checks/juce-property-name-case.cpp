#include <juce_mock.h>

namespace IDs
{
const juce::Identifier cutoff{ "cutoff" };
const juce::Identifier gain{ "gain" };
} // namespace IDs

void update(juce::ValueTree& state)
{
    state.setProperty("Cutoff", 1000, nullptr); // EXPECT_WARNING

    state.setProperty("cutoff", 500, nullptr);
    state.setProperty("x", 1, nullptr);
    state.setProperty("X", 1, nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    return { std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "drive", 1 }, "Drive", 0.0f, 1.0f, 0.5f),
             std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false) };
}

void attach(juce::AudioProcessorValueTreeState& apvts, juce::Slider& slider)
{
    juce::AudioProcessorValueTreeState::SliderAttachment attachment{ apvts, "Drive", slider }; // EXPECT_WARNING

    apvts.getRawParameterValue("BYPASS"); // EXPECT_WARNING

    apvts.getRawParameterValue("gain");
    apvts.getRawParameterValue("drive");
}
