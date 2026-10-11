#include <juce_mock.h>

using Resource = juce::WebBrowserComponent::Resource;

std::vector<std::byte> load(const juce::String& path);

std::optional<Resource> provide(const juce::String& url)
{
    if (url == "/")
        return Resource{ load("index.html"), "text/html" };

    if (url == "/main.js")
        return Resource{ .data = load("main.js") }; // EXPECT_WARNING

    if (url == "/style.css")
        return Resource{ load("style.css"), "" }; // EXPECT_WARNING

    if (url == "/data.bin")
        return Resource{ load("data.bin"), {} }; // EXPECT_WARNING

    return std::nullopt;
}

Resource withType(std::vector<std::byte> data, const juce::String& type)
{
    return { std::move(data), type };
}
