#include <juce_mock.h>

class NoPrograms : public juce::AudioProcessor
{
public:
    int getNumPrograms() override
    {
        return 0; // EXPECT_WARNING
    }
};

class NoProgramsInline : public juce::AudioProcessor
{
public:
    int getNumPrograms() override { return numPrograms; } // EXPECT_WARNING

private:
    static constexpr int numPrograms = 0;
};

class OneProgram : public juce::AudioProcessor
{
public:
    int getNumPrograms() override
    {
        return 1;
    }
};

class Presets : public juce::AudioProcessor
{
public:
    int getNumPrograms() override
    {
        return presetCount;
    }

private:
    int presetCount = 0;
};
