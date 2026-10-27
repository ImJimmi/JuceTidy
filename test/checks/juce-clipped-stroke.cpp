#include <juce_mock.h>

class Outlined : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 2.0f); // EXPECT_WARNING
        g.drawEllipse(getLocalBounds().toFloat(), 2.0f);                // EXPECT_WARNING

        const auto bounds = getLocalBounds().toFloat();
        g.drawEllipse(bounds, 2.0f); // EXPECT_WARNING

        const auto intBounds = getLocalBounds();
        g.drawEllipse(intBounds.toFloat(), 2.0f); // EXPECT_WARNING

        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 2.0f);
        g.drawEllipse(getLocalBounds().reduced(1).toFloat(), 2.0f);
        g.drawEllipse(bounds.reduced(1.0f), 2.0f);
        g.drawEllipse(getLocalBounds().toFloat().withSizeKeepingCentre(10.0f, 10.0f), 2.0f);
        g.drawRect(getLocalBounds().toFloat(), 2.0f);

        auto reducedBounds = getLocalBounds().toFloat();
        reducedBounds.reduce(1.0f, 1.0f);
        g.drawEllipse(reducedBounds, 2.0f);

        auto reassigned = getLocalBounds().toFloat();
        reassigned = reassigned.reduced(1.0f);
        g.drawEllipse(reassigned, 2.0f);

        g.drawEllipse(area, 2.0f);
    }

private:
    juce::Rectangle<float> area;
};

class StrokedPaths : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        juce::Path outline;
        outline.addRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        g.strokePath(outline, juce::PathStrokeType(2.0f)); // EXPECT_WARNING

        juce::Path box;
        box.addRectangle(getLocalBounds());
        g.strokePath(box, juce::PathStrokeType(2.0f)); // EXPECT_WARNING

        const auto bounds = getLocalBounds().toFloat();
        juce::Path circle;
        circle.addEllipse(bounds);
        g.strokePath(circle, juce::PathStrokeType(2.0f)); // EXPECT_WARNING

        juce::Path inset;
        inset.addRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f);
        g.strokePath(inset, juce::PathStrokeType(2.0f));

        juce::Path filled;
        filled.addRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        g.fillPath(filled);

        juce::Path transformed;
        transformed.addEllipse(bounds);
        transformed.applyTransform(juce::AffineTransform::translation(1.0f, 1.0f));
        g.strokePath(transformed, juce::PathStrokeType(2.0f));

        juce::Path drawnWithTransform;
        drawnWithTransform.addEllipse(bounds);
        g.strokePath(drawnWithTransform, juce::PathStrokeType(2.0f), juce::AffineTransform::translation(1.0f, 1.0f));

        g.strokePath(cachedPath, juce::PathStrokeType(2.0f));
    }

private:
    juce::Path cachedPath;
};

class OutlinedButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour&,
                              bool,
                              bool) override
    {
        g.drawRoundedRectangle(button.getLocalBounds().toFloat(), 4.0f, 1.0f); // EXPECT_WARNING
    }
};
