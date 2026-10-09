// OPTION: FlagMacros=^(JUCE|JucePlugin|MYPLUGIN)_

#include <juce_mock.h>

#define MYPLUGIN_FEATURE 1

#ifdef MYPLUGIN_FEATURE // EXPECT_WARNING
#endif

#ifdef OTHER_FEATURE
#endif
