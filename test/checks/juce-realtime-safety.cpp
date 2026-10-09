#include <juce_mock.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto* samples = new float[64]; // EXPECT_WARNING
        delete[] samples; // EXPECT_WARNING

        auto owned = std::make_unique<int>(0); // EXPECT_WARNING

        values.push_back(1.0f); // EXPECT_WARNING

        lookup[1] = 2; // EXPECT_WARNING

        array.add(1); // EXPECT_WARNING

        scratch.setSize(2, buffer.getNumSamples()); // EXPECT_WARNING

        scratch.setSize(2, buffer.getNumSamples(), false, false, true);

        const juce::String message = "Processing"; // EXPECT_WARNING

        const juce::String copy = message;
        const auto concatenated = message + "!"; // EXPECT_WARNING

        const juce::ScopedLock lock(criticalSection); // EXPECT_WARNING

        const std::lock_guard<std::mutex> guard(mutex); // EXPECT_WARNING

        mutex.lock(); // EXPECT_WARNING

        const juce::ScopedTryLock tryLock(criticalSection);
        const juce::GenericScopedLock<juce::SpinLock> spinLock(spin);

        juce::Thread::sleep(1); // EXPECT_WARNING

        juce::MessageManager::callAsync([] {}); // EXPECT_WARNING

        broadcaster.sendChangeMessage(); // EXPECT_WARNING

        if (auto* editor = getActiveEditor())
            editor->repaint(); // EXPECT_WARNING

        juce::Logger::writeToLog(copy); // EXPECT_WARNING

        std::printf("%d\n", 1); // EXPECT_WARNING

        std::cout << "processing"; // EXPECT_WARNING

        DBG("processing"); // EXPECT_WARNING

        const auto gain = apvts->getRawParameterValue("gain")->load(); // EXPECT_WARNING

        if (gain < 0.0f)
            throw std::runtime_error("negative gain"); // EXPECT_WARNING

        std::for_each(values.begin(), values.end(), [this](float value) {
            values.push_back(value); // EXPECT_WARNING
        });

        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, 1000.0f); // EXPECT_WARNING

        const auto bufferCopy = buffer; // EXPECT_WARNING
        scratch = buffer; // EXPECT_WARNING

        state.getProperty("gain"); // EXPECT_WARNING
        state.getProperty(gainId); // EXPECT_WARNING
        buffer.applyGain(cachedMix.get()); // EXPECT_WARNING
        const float mix = cachedMix; // EXPECT_WARNING
        (void) mix;
        (void) bufferCopy;

        jassert(buffer.getNumChannels() > 0);
        buffer.applyGain(cachedGain->load());
        (void) concatenated;
        (void) owned;
    }

    void prepareToPlay(double, int samplesPerBlock) override
    {
        values.reserve(static_cast<size_t>(samplesPerBlock));
        scratch.setSize(2, samplesPerBlock);
        DBG("prepared");
    }

private:
    std::vector<float> values;
    std::map<int, int> lookup;
    juce::Array<int> array;
    juce::AudioBuffer<float> scratch;
    juce::CriticalSection criticalSection;
    juce::SpinLock spin;
    std::mutex mutex;
    juce::ChangeBroadcaster broadcaster;
    juce::AudioProcessorValueTreeState* apvts = nullptr;
    std::atomic<float>* cachedGain = nullptr;
    juce::dsp::IIR::Filter<float> filter;
    juce::ValueTree state;
    const juce::Identifier gainId{ "gain" };
    juce::CachedValue<float> cachedMix;
};

class Source : public juce::AudioSource
{
public:
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        info.buffer->clear();
        juce::Logger::writeToLog("block"); // EXPECT_WARNING
    }
};
