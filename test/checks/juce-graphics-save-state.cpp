#include <juce_mock.h>

void unmatched(juce::Graphics& g)
{
    g.saveState(); // EXPECT_WARNING
    g.fillAll();
}

void earlyReturn(juce::Graphics& g, bool skip)
{
    g.saveState();

    if (skip)
        return; // EXPECT_WARNING

    g.fillAll();
    g.restoreState();
}

void balanced(juce::Graphics& g)
{
    g.saveState();
    g.fillAll();
    g.restoreState();
}

void scoped(juce::Graphics& g)
{
    const juce::Graphics::ScopedSaveState state(g);
    g.fillAll();
}
