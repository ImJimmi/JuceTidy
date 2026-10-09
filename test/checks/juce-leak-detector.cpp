#include <juce_mock.h>

class MissingDetector : public juce::Component // EXPECT_WARNING
{
public:
    void paint(juce::Graphics&) override;

private:
    int value = 0;
};

class Processor : public juce::AudioProcessor // EXPECT_WARNING
{
public:
    Processor(const Processor&) = delete;
};

struct Inline : juce::Component {}; // EXPECT_WARNING

class Editor : public juce::AudioProcessorEditor
{
public:
    class Nested : public juce::Component // EXPECT_WARNING
    {
    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor)
};

class WithLeakDetector : public juce::Component
{
    JUCE_LEAK_DETECTOR (WithLeakDetector)
};

template <typename Type>
class Templated : public juce::Component
{
public:
    Templated() = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Templated)
};

Templated<int> instantiation;

class NotAComponent
{
};

void localClassesCantHaveLeakDetectors()
{
    struct Local : juce::Component
    {
    };
}
