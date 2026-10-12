#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    void prepareToPlay(double sampleRate, int blockSize) override
    {
        setLatencySamples(64);
        setRateAndBufferSizeDetails(sampleRate, blockSize);
    }

    std::function<void()> onAnalyserChanged;
};

class Editor : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor& p)
        : juce::AudioProcessorEditor{ p }
        , audioProcessor{ p }
    {
        audioProcessor.setLatencySamples(128); // EXPECT_WARNING
        processor.setPlayConfigDetails(2, 2, 44100.0, 512); // EXPECT_WARNING

        juce::MessageManager::callAsync([this] {
            audioProcessor.setRateAndBufferSizeDetails(48000.0, 256); // EXPECT_WARNING
        });

        const auto sampleRate = audioProcessor.getSampleRate();
        (void) sampleRate;
    }

private:
    Processor& audioProcessor;
};

class PointerEditor : public juce::AudioProcessorEditor
{
public:
    explicit PointerEditor(Processor* p)
        : juce::AudioProcessorEditor{ *p }
    {
        p->setLatencySamples(32); // EXPECT_WARNING
    }
};

void configure(Processor& processor)
{
    processor.setLatencySamples(16);
}
