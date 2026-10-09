#ifdef _WIN32
#endif

#include <juce_mock.h>

#ifdef _WIN32 // EXPECT_WARNING
#endif

#ifndef NDEBUG // EXPECT_WARNING
#endif

#if defined(__APPLE__) // EXPECT_WARNING
#elif defined (__linux__) && ! defined (__ANDROID__) // EXPECT_WARNING
#endif

#if _WIN64 || __x86_64__ // EXPECT_WARNING
#endif

#if ! defined (NDEBUG) && __clang__ // EXPECT_WARNING
#endif

#if defined (_MSC_VER) && _MSC_VER >= 1920 // EXPECT_WARNING
#endif

#ifndef DEBUG
 #define DEBUG 0
#endif

#if JUCE_MAC && JUCE_DEBUG && __has_include(<linux/version.h>)
#endif
