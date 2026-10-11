#include <juce_mock.h>

class BuffersItself : public juce::Component
{
public:
    BuffersItself()
    {
        setBufferedToImage(true); // EXPECT_WARNING
        addAndMakeVisible(foreground);
    }

private:
    juce::Label foreground;
};

class AddsChildElsewhere : public juce::Component
{
public:
    AddsChildElsewhere()
    {
        setBufferedToImage(true); // EXPECT_WARNING
    }

    void showOverlay()
    {
        addChildComponent(overlay);
    }

private:
    juce::Label overlay;
};

class BuffersChildWithChildren : public juce::Component
{
public:
    BuffersChildWithChildren()
    {
        panel.setBufferedToImage(true); // EXPECT_WARNING
        panel.addAndMakeVisible(label);
        addAndMakeVisible(panel);
    }

private:
    juce::Component panel;
    juce::Label label;
};

class BuffersSibling : public juce::Component
{
public:
    BuffersSibling()
    {
        background.setBufferedToImage(true);
        addAndMakeVisible(background);
        addAndMakeVisible(foreground);
    }

private:
    juce::Component background;
    juce::Label foreground;
};

class BuffersLeaf : public juce::Component
{
public:
    BuffersLeaf()
    {
        setBufferedToImage(true);
    }
};

class StopsBuffering : public juce::Component
{
public:
    StopsBuffering()
    {
        setBufferedToImage(false);
        addAndMakeVisible(label);
    }

private:
    juce::Label label;
};
