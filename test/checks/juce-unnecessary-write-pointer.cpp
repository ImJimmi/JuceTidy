#include <juce_mock.h>

#include <span>

void read(const float* data, int numSamples);
void write(float* data, int numSamples);
void readAll(const float* const* data, int numChannels, int numSamples);
void writeAll(float* const* data, int numChannels, int numSamples);
void readSpan(std::span<const float> data);
void writeSpan(std::span<float> data);

float sumOf(juce::AudioBuffer<float>& buffer)
{
    auto sum = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* data = buffer.getWritePointer(channel); // EXPECT_WARNING

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            sum += data[sample];
    }

    return sum;
}

void onlyReads(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<double>& doubles)
{
    auto* data = buffer.getWritePointer(0); // EXPECT_WARNING
    [[maybe_unused]] const auto first = data[0];
    [[maybe_unused]] const auto second = *(data + 1);
    read(data, buffer.getNumSamples());

    read(buffer.getWritePointer(1), buffer.getNumSamples());             // EXPECT_WARNING
    [[maybe_unused]] const auto peak = buffer.getWritePointer(1, 4)[0];  // EXPECT_WARNING
    [[maybe_unused]] const auto precise = doubles.getWritePointer(0)[0]; // EXPECT_WARNING

    auto* channels = buffer.getArrayOfWritePointers(); // EXPECT_WARNING
    [[maybe_unused]] const auto left = channels[0][0];
    const float* right = channels[1];
    read(right, buffer.getNumSamples());

    readAll(buffer.getArrayOfWritePointers(), 2, buffer.getNumSamples()); // EXPECT_WARNING

    read(buffer.getReadPointer(0), buffer.getNumSamples());
    readAll(buffer.getArrayOfReadPointers(), 2, buffer.getNumSamples());

    const std::span channel{
        buffer.getWritePointer(0), // EXPECT_WARNING
        static_cast<std::size_t>(buffer.getNumSamples()),
    };
    auto* channelData = std::data(channel);
    readAll(&channelData, 1, static_cast<int>(std::size(channel)));
}

float readsThroughSpans(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = static_cast<std::size_t>(buffer.getNumSamples());
    auto sum = 0.0f;

    std::span<float> samples{ buffer.getWritePointer(0), numSamples }; // EXPECT_WARNING

    for (const auto sample : samples)
        sum += sample;

    for (const auto& sample : samples)
        sum += sample;

    sum += samples[0] + samples.front() + samples.back();
    read(samples.data(), static_cast<int>(samples.size()));
    read(std::data(samples), static_cast<int>(std::size(samples)));
    readSpan(samples);

    if (!samples.empty())
        sum += samples.subspan(1)[0] + samples.first(1)[0] + samples.last(1)[0];

    const auto tail = samples.subspan(1);
    sum += tail[0];

    const std::span<float> copy = samples;
    sum += copy[0];

    const std::span<const float> constant{ buffer.getWritePointer(1), numSamples }; // EXPECT_WARNING
    sum += constant[0];

    readSpan(std::span{ buffer.getWritePointer(1), numSamples }); // EXPECT_WARNING

    auto* data = buffer.getWritePointer(1); // EXPECT_WARNING
    const std::span fromVariable{ data, numSamples };
    sum += fromVariable[0];

    auto* channels = buffer.getArrayOfWritePointers(); // EXPECT_WARNING
    const std::span left{ channels[0], numSamples };
    sum += left[0];

    return sum;
}

const float* exposeForReading(juce::AudioBuffer<float>& buffer)
{
    return buffer.getWritePointer(0); // EXPECT_WARNING
}

float* exposeForWriting(juce::AudioBuffer<float>& buffer)
{
    return buffer.getWritePointer(0);
}

void writes(juce::AudioBuffer<float>& buffer, float gain)
{
    auto* data = buffer.getWritePointer(0);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        data[sample] *= gain;

    buffer.getWritePointer(1)[0] = 0.0f;
    write(buffer.getWritePointer(0), buffer.getNumSamples());

    auto* original = buffer.getWritePointer(1);
    float* alias = original;
    alias[0] = 1.0f;

    float& first = buffer.getWritePointer(0)[0];
    first = 1.0f;

    auto* captured = buffer.getWritePointer(0);
    [&] {
        captured[0] = 0.0f;
    }();

    auto* channels = buffer.getArrayOfWritePointers();
    channels[0][0] = 0.0f;

    auto* others = buffer.getArrayOfWritePointers();
    auto* left = others[0];
    ++left[0];

    writeAll(buffer.getArrayOfWritePointers(), 2, buffer.getNumSamples());

    auto* addressed = buffer.getWritePointer(0);
    writeAll(&addressed, 1, buffer.getNumSamples());

    juce::Reverb reverb;
    auto* stereo = buffer.getArrayOfWritePointers();
    reverb.processStereo(stereo[0], stereo[1], buffer.getNumSamples());

    buffer.getWritePointer(0);
}

void writesThroughSpans(juce::AudioBuffer<float>& buffer, float gain)
{
    const auto numSamples = static_cast<std::size_t>(buffer.getNumSamples());

    const std::span ranged{ buffer.getWritePointer(0), numSamples };

    for (auto& sample : ranged)
        sample *= gain;

    std::span<float> indexed{ buffer.getWritePointer(0), numSamples };
    indexed[0] = 0.0f;

    std::span<float> fronted{ buffer.getWritePointer(0), numSamples };
    fronted.front() *= gain;

    std::span<float> pointed{ buffer.getWritePointer(0), numSamples };
    write(pointed.data(), buffer.getNumSamples());

    std::span<float> freePointed{ buffer.getWritePointer(0), numSamples };
    write(std::data(freePointed), buffer.getNumSamples());

    std::span<float> sliced{ buffer.getWritePointer(0), numSamples };
    sliced.subspan(1)[0] = 0.0f;

    std::span<float> copied{ buffer.getWritePointer(0), numSamples };
    auto copy = copied;
    copy[0] = 0.0f;

    std::span<float> passed{ buffer.getWritePointer(0), numSamples };
    writeSpan(passed);

    std::span<float> iterated{ buffer.getWritePointer(0), numSamples };
    *iterated.begin() = 0.0f;

    writeSpan(std::span{ buffer.getWritePointer(0), numSamples });

    auto* data = buffer.getWritePointer(0);
    const std::span fromVariable{ data, numSamples };
    fromVariable[0] = 0.0f;

    auto* channels = buffer.getArrayOfWritePointers();
    const std::span left{ channels[0], numSamples };
    left[0] = 0.0f;
}

class Processor
{
public:
    void prepare(juce::AudioBuffer<float>& buffer)
    {
        cached = buffer.getWritePointer(0);
    }

private:
    float* cached = nullptr;
};

template <typename ProcessorType>
void process(juce::AudioBuffer<float>& buffer, ProcessorType& processor)
{
    processor.process(buffer.getWritePointer(0));
}
