#include <juce_mock.h>

class Settings
{
public:
    explicit Settings(juce::ValueTree& tree)
        : boundInInitialiser(tree, "gain", nullptr)
    {
        boundLater.referTo(tree, "mix", nullptr);
    }

    float read() const
    {
        return boundInInitialiser.get() + boundLater.get() + unbound.get() + inClass.get();
    }

private:
    juce::ValueTree state;
    juce::CachedValue<float> boundInInitialiser;
    juce::CachedValue<float> boundLater;
    juce::CachedValue<float> unbound; // EXPECT_WARNING
    juce::CachedValue<float> inClass{ state, "pan", nullptr };
};

class DefinedElsewhere
{
public:
    void attach(juce::ValueTree& tree);

private:
    juce::CachedValue<int> value;
};
