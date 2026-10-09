#include <juce_mock.h>

class MissingDenormals : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override // EXPECT_WARNING
    {
        buffer.applyGain(0.5f);
    }
};

class DisablesDenormals : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        buffer.applyGain(0.5f);
    }

    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi) override
    {
        process(buffer, midi);
    }

private:
    template <typename SampleType>
    void process(juce::AudioBuffer<SampleType>& buffer, juce::MidiBuffer&)
    {
        juce::ScopedNoDenormals noDenormals;
        buffer.clear();
    }
};

class Intermediate : public juce::AudioProcessor
{
};

class Derived : public Intermediate
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
};

void Derived::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) // EXPECT_WARNING
{
    buffer.clear();
}

class UsesFloatVectorOperations : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();
        buffer.clear();
    }
};
