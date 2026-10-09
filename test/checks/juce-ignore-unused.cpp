#include <juce_mock.h>

void parameter(int unusedParameter)
{
    juce::ignoreUnused(unusedParameter); // EXPECT_WARNING
}

void locals(float gain)
{
    const auto first = gain * 2.0f;
    auto second = 0.0f;
    juce::ignoreUnused(first, second); // EXPECT_WARNING
}

void expression(int value)
{
    juce::ignoreUnused(value + 1); // EXPECT_WARNING
}
