#include <juce_mock.h>

class FilterState
{
public:
    explicit FilterState(juce::ValueTree tree)
        : state(tree)
    {
        state.setProperty("cutoff", 1000, nullptr);
    }

    void setCutoff(float cutoff)
    {
        state.setProperty("cutoff", cutoff, &undoManager);
    }

    void setResonance(float resonance)
    {
        state.setProperty("resonance", resonance, nullptr); // EXPECT_WARNING
    }

    void removeBand(const juce::ValueTree& band);

private:
    juce::ValueTree state;
    juce::UndoManager undoManager;
};

void FilterState::removeBand(const juce::ValueTree& band)
{
    state.removeChild(band, nullptr); // EXPECT_WARNING
}

class WindowSettings
{
public:
    void setSize(int width, int height)
    {
        state.setProperty("width", width, nullptr);
        state.setProperty("height", height, nullptr);
    }

private:
    juce::ValueTree state;
};
