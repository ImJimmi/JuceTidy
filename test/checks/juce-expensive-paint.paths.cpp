#include <juce_mock.h>

namespace
{
constexpr auto cornerSize = 4.0f;
const juce::Colour backgroundColour{ 0xff202020 };
float mutableWidth = 10.0f;
} // namespace

class Knob : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        juce::Path arrow; // EXPECT_WARNING
        arrow.startNewSubPath(0.0f, 0.0f);
        arrow.lineTo(10.0f, 5.0f);
        arrow.lineTo(0.0f, 10.0f);
        arrow.closeSubPath();
        g.fillPath(arrow);

        juce::Path badge; // EXPECT_WARNING
        badge.addRoundedRectangle(0.0f, 0.0f, 20.0f, 10.0f, cornerSize);
        g.fillPath(badge);

        juce::Path outline;
        outline.addRoundedRectangle(0.0f, 0.0f, static_cast<float>(getWidth()), static_cast<float>(getHeight()), cornerSize);
        g.fillPath(outline);

        const auto width = static_cast<float>(getWidth());
        juce::Path line;
        line.startNewSubPath(0.0f, 0.0f);
        line.lineTo(width, 0.0f);
        g.fillPath(line);

        juce::Path meter;
        meter.addRoundedRectangle(0.0f, 0.0f, level, 10.0f, cornerSize);
        g.fillPath(meter);

        juce::Path global;
        global.lineTo(mutableWidth, 0.0f);
        g.fillPath(global);

        juce::Path copied = cachedPath;
        copied.lineTo(1.0f, 1.0f);
        g.fillPath(copied);

        juce::Path built;
        buildPath(built);
        g.fillPath(built);

        juce::Path empty;
        g.fillPath(empty);

        juce::Path original; // EXPECT_WARNING
        original.lineTo(1.0f, 1.0f);
        const juce::Path duplicate{ original };
        g.fillPath(duplicate);

        const juce::ColourGradient shine{ backgroundColour, 0.0f, 0.0f, juce::Colour(0xff000000), 0.0f, 100.0f, false }; // EXPECT_WARNING
        g.setGradientFill(shine);

        g.setGradientFill(juce::ColourGradient::vertical(backgroundColour, 0.0f, juce::Colour(0xff000000), 100.0f)); // EXPECT_WARNING

        g.setGradientFill(juce::ColourGradient::vertical(backgroundColour, 0.0f, juce::Colour(0xff000000), static_cast<float>(getHeight())));

        g.setGradientFill(gradient);
    }

private:
    void buildPath(juce::Path& path) const;

    float level = 0.0f;
    juce::Path cachedPath;
    juce::ColourGradient gradient;
};
