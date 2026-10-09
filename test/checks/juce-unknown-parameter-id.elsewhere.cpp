#include <juce_mock.h>

void connect(juce::AudioProcessorValueTreeState& apvts)
{
    apvts.getRawParameterValue("declaredInAnotherFile");
}
