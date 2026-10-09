#include <juce_mock.h>

void update(juce::ValueTree& state)
{
    state.getChildWithName("Filter").setProperty("cutoff", 1000, nullptr); // EXPECT_WARNING

    state.getParent().appendChild(juce::ValueTree(juce::Identifier("Sibling")), nullptr); // EXPECT_WARNING

    auto reverb = state.getChildWithName("Reverb");
    reverb.setProperty("mix", 0.5, nullptr); // EXPECT_WARNING

    auto delay = state.getChildWithName("Delay");

    if (delay.isValid())
        delay.setProperty("time", 0.25, nullptr);

    state.getOrCreateChildWithName("Chorus", nullptr).setProperty("depth", 0.5, nullptr);
    (void) state.getChildWithName("Filter").getProperty("cutoff");
}
