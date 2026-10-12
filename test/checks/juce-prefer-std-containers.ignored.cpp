// OPTION: IgnoredTypes=juce::CriticalSection;::juce::GenericScopedLock
#include <juce_mock.h>

void useLock(juce::CriticalSection& section, const juce::Array<int>& values) // EXPECT_WARNING
{
    const juce::ScopedLock scopedLock(section);
    (void) values;
}
