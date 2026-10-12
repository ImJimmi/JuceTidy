#include <juce_mock.h>

#include <vector>

struct Receiver
{
    virtual ~Receiver() = default;
    virtual void receive(const juce::Array<int>& values) = 0; // EXPECT_WARNING
};

class Holder : public Receiver
{
public:
    void set(const juce::Array<int>& values); // EXPECT_WARNING
    juce::Array<float> get() const; // EXPECT_WARNING
    void receive(const juce::Array<int>& values) override;

private:
    juce::Array<int> numbers; // EXPECT_WARNING
    juce::OwnedArray<juce::Component> children; // EXPECT_WARNING
    juce::HashMap<int, float> lookup; // EXPECT_WARNING
    juce::SortedSet<int> sorted; // EXPECT_WARNING
    juce::CriticalSection lock; // EXPECT_WARNING
    juce::HeapBlock<float> block; // EXPECT_WARNING
    juce::Array<int>* pointer = nullptr; // EXPECT_WARNING
    std::vector<int> values;
};

void useLock(juce::CriticalSection& section) // EXPECT_WARNING
{
    const juce::ScopedLock scopedLock(section); // EXPECT_WARNING
    auto copy = juce::Array<int>{};
    (void) copy;
}

juce::WebBrowserComponent::Options withNative(const juce::WebBrowserComponent::Options& options)
{
    return options.withNativeFunction("greet", [](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion) {});
}

class Views
{
public:
    void read(const juce::MemoryBlock& data); // EXPECT_WARNING
    void write(juce::MemoryBlock& data); // EXPECT_WARNING
    juce::MemoryBlock copy() const; // EXPECT_WARNING

private:
    juce::MemoryBlock owned; // EXPECT_WARNING
    juce::Span<const float> samples; // EXPECT_WARNING
    juce::Optional<int> maybe; // EXPECT_WARNING
    juce::Atomic<int> counter; // EXPECT_WARNING
    juce::uint8 byte = 0; // EXPECT_WARNING
    juce::int64 big = 0; // EXPECT_WARNING
    std::vector<juce::uint32> words; // EXPECT_WARNING
};

void ownership()
{
    const auto value = static_cast<juce::uint16>(3); // EXPECT_WARNING
    (void) value;
}
