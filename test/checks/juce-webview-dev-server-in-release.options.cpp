// OPTION: DebugMacros=JUCE_DEBUG;USE_DEV_SERVER

#include <juce_mock.h>

#define USE_DEV_SERVER 0
#define USE_LOCAL_SERVER 1

#if USE_DEV_SERVER
static constexpr auto server = "http://localhost:5173/";
#else
static constexpr auto server = "http://localhost:3000/";
#endif

#if USE_LOCAL_SERVER
static constexpr auto localServer = "http://localhost:4000/";
#endif

#undef USE_DEV_SERVER
#define USE_DEV_SERVER 1

#if USE_DEV_SERVER
static constexpr auto devServer = "http://localhost:5173/";
#endif

#if ! USE_DEV_SERVER
#else
static constexpr auto negatedDevServer = "http://localhost:5174/";
#endif

class Browser : public juce::WebBrowserComponent
{
public:
    bool pageAboutToLoad(const juce::String& url) override
    {
        return url == server // EXPECT_WARNING
            || url == localServer // EXPECT_WARNING
            || url == devServer
            || url == negatedDevServer;
    }
};
