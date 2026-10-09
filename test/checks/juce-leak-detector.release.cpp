// OPTION: BaseClasses=::juce::Component;::juce::Thread
#include <juce_mock.h>

#undef JUCE_LEAK_DETECTOR
#define JUCE_LEAK_DETECTOR(OwnerClass)

class ReleaseBuild : public juce::Component
{
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReleaseBuild)
};

class Worker : public juce::Thread // EXPECT_WARNING
{
    void run() override;
};
