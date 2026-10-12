#include <juce_mock.h>

#include <algorithm>
#include <limits>

void layout(juce::Rectangle<float>& constraints, juce::Component& component)
{
    constraints.setHeight(std::numeric_limits<float>::max()); // EXPECT_WARNING
    constraints.setWidth(-std::numeric_limits<float>::max()); // EXPECT_WARNING
    constraints.setWidth(std::numeric_limits<double>::lowest()); // EXPECT_WARNING

    const juce::Rectangle<float> bounds{ 0.0f, 0.0f, std::numeric_limits<float>::infinity(), 10.0f }; // EXPECT_WARNING
    (void) bounds;

    constraints.setWidth(static_cast<float>(std::numeric_limits<juce::uint16>::max()));
    component.setSize(std::numeric_limits<int>::max(), 10);

    const auto clamped = std::min(std::numeric_limits<float>::max(), 1.0f);
    (void) clamped;
}
