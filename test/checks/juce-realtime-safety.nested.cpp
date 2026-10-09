#include <juce_mock.h>

#include <vector>

void processElsewhere(juce::AudioBuffer<float>& buffer);

namespace juce
{
inline void allocatesInsideJuce()
{
    (void) new int;
}
} // namespace juce

namespace dsp
{
class Filter
{
public:
    void process(juce::AudioBuffer<float>& buffer)
    {
        updateCoefficients();
        updateCoefficients();
        buffer.applyGain(gain);
    }

private:
    void updateCoefficients()
    {
        history.push_back(gain); // EXPECT_WARNING
    }

    std::vector<float> history;
    float gain = 1.0f;
};
} // namespace dsp

template <typename SampleType>
void resizeScratch(std::vector<SampleType>& scratch)
{
    scratch.resize(64); // EXPECT_WARNING
}

void logRecursively(int depth)
{
    if (depth > 0)
        logRecursively(depth - 1);

    DBG("depth"); // EXPECT_WARNING
}

struct Voice
{
    Voice()
        : samples(new float[16]) // EXPECT_WARNING
    {
    }

    float* samples;
};

class Engine
{
public:
    void render() [[clang::nonblocking]];

private:
    juce::CriticalSection lock;
};

void Engine::render()
{
    lock.enter(); // EXPECT_WARNING
}

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        filter.process(buffer);
        resizeScratch(scratch);
        logRecursively(2);
        engine.render();
        processElsewhere(buffer);
        juce::allocatesInsideJuce();

        Voice voice;
        (void) voice;

        auto applyFilter = [this, &buffer] { filter.process(buffer); };
        applyFilter();
    }

    void prepareToPlay(double, int) override
    {
        scratch.reserve(64);
        logRecursively(1);
    }

private:
    dsp::Filter filter;
    std::vector<float> scratch;
    Engine engine;
};
