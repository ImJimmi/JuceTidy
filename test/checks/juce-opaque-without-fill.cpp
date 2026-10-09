#include <juce_mock.h>

class Unfilled : public juce::Component
{
public:
    Unfilled()
    {
        setOpaque(true); // EXPECT_WARNING
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff000000));
    }
};

class NoPaint : public juce::Component
{
public:
    NoPaint()
    {
        setOpaque(true); // EXPECT_WARNING
    }
};

class Filled : public juce::Component
{
public:
    Filled()
    {
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff000000));
    }
};

void drawBackground(juce::Graphics& g);

class FilledByHelper : public juce::Component
{
public:
    FilledByHelper()
    {
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override
    {
        drawBackground(g);
    }
};

class InheritsPaint : public Filled
{
public:
    InheritsPaint()
    {
        setOpaque(true);
    }
};

class PaintedByJuce : public juce::Slider
{
public:
    PaintedByJuce()
    {
        setOpaque(true);
    }
};

class PaintDefinedElsewhere : public juce::Component
{
public:
    PaintDefinedElsewhere()
    {
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override;
};
