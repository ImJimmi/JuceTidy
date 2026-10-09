#include <juce_mock.h>

extern const char logoData[];
extern const int logoDataSize;

class Logo : public juce::Component
{
public:
    Logo()
        : drawable(juce::Drawable::createFromImageData(logoData, static_cast<size_t>(logoDataSize)))
    {
    }

    void paint(juce::Graphics& g) override
    {
        auto image = juce::ImageFileFormat::loadFrom(logoData, static_cast<size_t>(logoDataSize)); // EXPECT_WARNING
        const juce::Image scratch(juce::Image::ARGB, getWidth(), getHeight(), true); // EXPECT_WARNING
        const auto xml = juce::parseXML("<svg/>"); // EXPECT_WARNING

        g.drawImageAt(image, 0, 0);
        g.drawImageAt(juce::ImageCache::getFromMemory(logoData, logoDataSize), 0, 0);
        const juce::Image copy = cached;
        (void) scratch;
        (void) copy;
    }

private:
    std::unique_ptr<juce::Drawable> drawable;
    juce::Image cached;
};
