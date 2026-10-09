#include <juce_mock.h>

class MissingReset : public juce::Component
{
public:
    MissingReset()
    {
        setLookAndFeel(&lookAndFeel);

        slider.setLookAndFeel(&lookAndFeel); // EXPECT_WARNING

        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel); // EXPECT_WARNING
    }

    ~MissingReset() override
    {
        setLookAndFeel(nullptr);
    }

private:
    juce::Slider slider;
    juce::LookAndFeel_V4 lookAndFeel;
};

class ResetsEverything : public juce::Component
{
public:
    ResetsEverything()
    {
        setLookAndFeel(&lookAndFeel);
        slider.setLookAndFeel(&lookAndFeel);
    }

    ~ResetsEverything() override
    {
        slider.setLookAndFeel(nullptr);
        setLookAndFeel(nullptr);
    }

private:
    juce::LookAndFeel_V4 lookAndFeel;
    juce::Slider slider;
};

class NoDestructor : public juce::Component
{
public:
    NoDestructor()
    {
        setLookAndFeel(&lookAndFeel); // EXPECT_WARNING
    }

private:
    juce::LookAndFeel_V4 lookAndFeel;
};

class DefinedElsewhere : public juce::Component
{
public:
    DefinedElsewhere()
    {
        setLookAndFeel(&lookAndFeel);
    }

    ~DefinedElsewhere() override;

private:
    juce::LookAndFeel_V4 lookAndFeel;
};

class DeclaredInSafeOrder : public juce::Component
{
public:
    DeclaredInSafeOrder()
    {
        slider.setLookAndFeel(&lookAndFeel);
        earlierSlider.setLookAndFeel(&lookAndFeel); // EXPECT_WARNING
        setLookAndFeel(&lookAndFeel); // EXPECT_WARNING
    }

private:
    juce::Slider earlierSlider;
    juce::LookAndFeel_V4 lookAndFeel;
    juce::Slider slider;
};
