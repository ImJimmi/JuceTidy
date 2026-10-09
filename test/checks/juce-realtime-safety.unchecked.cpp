// OPTION: ReportUncheckedCalls=true
#include <juce_mock.h>

void processElsewhere(juce::AudioBuffer<float>& buffer);
void trustedElsewhere(juce::AudioBuffer<float>& buffer) [[clang::nonblocking]];

struct BlockListener
{
    virtual ~BlockListener() = default;
    virtual void blockProcessed() = 0;
};

void helper(juce::AudioBuffer<float>& buffer)
{
    processElsewhere(buffer); // EXPECT_WARNING
}

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        processElsewhere(buffer); // EXPECT_WARNING
        listener->blockProcessed(); // EXPECT_WARNING
        trustedElsewhere(buffer);
        buffer.applyGain(0.5f);
        helper(buffer);
    }

private:
    BlockListener* listener = nullptr;
};
