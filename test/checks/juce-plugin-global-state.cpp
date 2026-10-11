#include <juce_mock.h>

class PresetManager
{
public:
    JUCE_DECLARE_SINGLETON(PresetManager, false) // EXPECT_WARNING

    static int instanceCount; // EXPECT_WARNING
    static const int maxPresets;
    static constexpr int defaultPreset = 0;
    static inline thread_local int currentPreset = 0; // EXPECT_WARNING
};

JUCE_IMPLEMENT_SINGLETON(PresetManager)

int PresetManager::instanceCount = 0;
const int PresetManager::maxPresets = 128;

int globalCounter = 0;

juce::String getPluginName()
{
    static juce::String name; // EXPECT_WARNING
    static const juce::String version{ "1.0.0" };
    static constexpr int build = 42;
    static const auto& alias = name;
    const juce::String local{ "local" };

    (void) version;
    (void) build;
    (void) alias;
    (void) local;

    return name;
}

template <typename T>
T& getDefault()
{
    static T value; // EXPECT_WARNING
    return value;
}

void useDefault()
{
    getDefault<int>() = 1;
    getDefault<float>() = 1.0f;
}
