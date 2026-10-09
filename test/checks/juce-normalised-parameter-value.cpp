#include <juce_mock.h>

float read(juce::AudioProcessorValueTreeState& apvts, juce::AudioParameterFloat& gain, juce::AudioProcessorParameter& generic)
{
    auto* parameter = apvts.getParameter("gain");
    const auto value = parameter->getValue(); // EXPECT_WARNING

    const juce::RangedAudioParameter& ranged = gain;
    const auto other = ranged.getValue(); // EXPECT_WARNING

    const auto real = parameter->convertFrom0to1(parameter->getValue());
    const auto direct = gain.get();
    const auto normalised = generic.getValue();

    return value + other + real + direct + normalised;
}
