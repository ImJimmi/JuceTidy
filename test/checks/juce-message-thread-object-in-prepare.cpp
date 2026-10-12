#include <juce_mock.h>

#include <memory>
#include <optional>
#include <vector>

class Analyser : private juce::Timer
{
public:
    void timerCallback() override
    {
    }
};

struct Graph
{
    Analyser analyser;
    float gain = 1.0f;
};

struct Filter
{
    float cutoff = 1000.0f;
};

class Processor : public juce::AudioProcessor
{
public:
    Processor()
        : graph{ std::make_unique<Graph>() }
    {
    }

    void prepareToPlay(double, int) override
    {
        graph = std::make_unique<Graph>(); // EXPECT_WARNING
        filter = std::make_unique<Filter>();
        buffer.resize(16);
        rebuild();
    }

    void releaseResources() override
    {
        graph.reset(); // EXPECT_WARNING
        filter.reset();
    }

    void numChannelsChanged() override
    {
        optionalAnalyser = std::nullopt; // EXPECT_WARNING
    }

private:
    void rebuild()
    {
        Analyser local; // EXPECT_WARNING
        (void) local;
    }

    std::unique_ptr<Graph> graph;
    std::unique_ptr<Filter> filter;
    std::optional<Analyser> optionalAnalyser;
    std::vector<float> buffer;
};
