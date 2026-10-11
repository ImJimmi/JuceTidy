#include <juce_mock.h>

using Options = juce::WebBrowserComponent::Options;

class SinglePageBrowser : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;

    bool pageAboutToLoad(const juce::String& newURL) override
    {
        return newURL == getResourceProviderRoot();
    }
};

class Restricted : public SinglePageBrowser
{
public:
    using SinglePageBrowser::SinglePageBrowser;
};

class Unrestricted : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;
};

class SelfConfigured : public juce::WebBrowserComponent
{
public:
    SelfConfigured()
        : juce::WebBrowserComponent{ Options{}.withNativeIntegrationEnabled() } // EXPECT_WARNING
    {
    }
};

class SelfRestricted : public juce::WebBrowserComponent
{
public:
    SelfRestricted()
        : juce::WebBrowserComponent{ Options{}.withNativeIntegrationEnabled() }
    {
    }

    bool pageAboutToLoad(const juce::String& newURL) override
    {
        return newURL == getResourceProviderRoot();
    }
};

class MemberConfigured : public juce::Component
{
public:
    MemberConfigured()
        : browser{ Options{}.withNativeIntegrationEnabled() } // EXPECT_WARNING
    {
    }

private:
    juce::WebBrowserComponent browser;
};

static Options integratedOptions()
{
    return Options{}.withNativeIntegrationEnabled();
}

class Editor : public juce::Component
{
    juce::WebBrowserComponent plain{ Options{}.withNativeIntegrationEnabled() }; // EXPECT_WARNING
    Unrestricted subclass{ integratedOptions() }; // EXPECT_WARNING
    std::unique_ptr<juce::WebBrowserComponent> owned = std::make_unique<juce::WebBrowserComponent>(integratedOptions()); // EXPECT_WARNING
    SinglePageBrowser restricted{ Options{}.withNativeIntegrationEnabled() };
    Restricted inherited{ integratedOptions() };
    juce::WebBrowserComponent disabled{ Options{}.withNativeIntegrationEnabled(false) };
    juce::WebBrowserComponent browsing{ Options{}.withUserAgent("JUCE") };
};
