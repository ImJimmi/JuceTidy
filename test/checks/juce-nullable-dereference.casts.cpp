// OPTION: CheckDynamicCasts=false
#include <juce_mock.h>

void casts(juce::Component& component)
{
    (void) dynamic_cast<juce::Slider*>(&component)->getValue();
}
