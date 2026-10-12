#include <juce_mock.h>

void constants()
{
    [[maybe_unused]] const auto a = juce::MathConstants<float>::pi; // EXPECT_WARNING
    [[maybe_unused]] const auto b = juce::MathConstants<double>::euler; // EXPECT_WARNING
    [[maybe_unused]] const auto c = 1.0f / juce::MathConstants<float>::twoPi; // EXPECT_WARNING
    [[maybe_unused]] const auto d = juce::MathConstants<double>::halfPi; // EXPECT_WARNING
    [[maybe_unused]] const auto e = juce::MathConstants<long double>::sqrt2; // EXPECT_WARNING
}
