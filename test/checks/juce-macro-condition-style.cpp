#include <juce_mock.h>

#define JucePlugin_Build_VST3 0
#define JucePlugin_Build_AU 1
#define JucePlugin_Name "Test"

#ifdef JucePlugin_Build_VST3 // EXPECT_WARNING
#endif

#ifndef JucePlugin_Build_AU // EXPECT_WARNING
#endif

#if defined(JUCE_DEBUG) && !defined(JUCE_WINDOWS) // EXPECT_WARNING
#endif

#if JucePlugin_Build_AU == 1 || 0 != JucePlugin_Build_VST3 // EXPECT_WARNING
#elif JUCE_DEBUG == 0 // EXPECT_WARNING
#endif

#ifndef JUCE_USE_CUSTOM_THING
    #define JUCE_USE_CUSTOM_THING 0
#endif

#ifdef JUCE_OVERRIDE_ME
    // Allow the project to override this.
    #undef JUCE_OVERRIDE_ME
#endif

#ifdef JucePlugin_Name
#endif

#ifndef JucePlugin_PreferredChannelConfigurations
#endif

#if defined(JUCE_MODULE_AVAILABLE_juce_dsp) // EXPECT_WARNING
#endif

#if defined(_WIN32) || defined(SOMETHING) || SOMETHING_ELSE == 1
#endif

#if JUCE_MAC && JucePlugin_Build_AU && !JUCE_WINDOWS
#endif
