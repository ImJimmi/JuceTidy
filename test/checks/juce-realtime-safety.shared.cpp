#include <juce_mock.h>

#include <atomic>
#include <memory>

struct Analyser
{
    void push(float sample);
};

struct Settings
{
    double values[8];
};

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        if (auto analyser = weakAnalyser.lock()) // EXPECT_WARNING
            analyser->push(0.0f);

        auto copy = sharedAnalyser; // EXPECT_WARNING
        sharedAnalyser.reset(); // EXPECT_WARNING
        sharedAnalyser = nullptr; // EXPECT_WARNING

        const auto& reference = sharedAnalyser;
        reference->push(1.0f);

        const auto current = settings.load(); // EXPECT_WARNING
        settings.store(current); // EXPECT_WARNING
        const Settings converted = settings; // EXPECT_WARNING
        (void) converted;

        const auto gainValue = gain.load();
        gain = gainValue;
        gain.store(gainValue);
    }

private:
    std::weak_ptr<Analyser> weakAnalyser;
    std::shared_ptr<Analyser> sharedAnalyser;
    std::atomic<Settings> settings;
    std::atomic<float> gain;
};
