#include <juce_mock.h>

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    return { std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "gain", 1 }, "Gain", 0.0f, 1.0f, 0.5f),
             std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false),
             std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 1.0f) };
}

void connect(juce::AudioProcessorValueTreeState& apvts, juce::Slider& slider, juce::AudioProcessorValueTreeState::Listener& listener)
{
    apvts.getRawParameterValue("gain");
    apvts.getRawParameterValue("gian"); // EXPECT_WARNING

    apvts.getParameter("output"); // EXPECT_WARNING

    apvts.addParameterListener("bypass", &listener);

    juce::AudioProcessorValueTreeState::SliderAttachment attachment{ apvts, "Mix", slider }; // EXPECT_WARNING
}
