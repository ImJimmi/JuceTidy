#include <juce_mock.h>

class MissingDetach : public juce::Component
{
public:
    MissingDetach()
    {
        openGLContext.attachTo(*this); // EXPECT_WARNING
    }

private:
    juce::OpenGLContext openGLContext;
};

class DetachesOtherContext : public juce::Component
{
public:
    DetachesOtherContext()
    {
        main.attachTo(*this); // EXPECT_WARNING
    }

    ~DetachesOtherContext() override
    {
        other.detach();
    }

private:
    juce::OpenGLContext main;
    juce::OpenGLContext other;
};

class Detaches : public juce::Component
{
public:
    Detaches()
    {
        openGLContext.attachTo(*this);
    }

    ~Detaches() override
    {
        openGLContext.detach();
    }

private:
    juce::OpenGLContext openGLContext;
};

class DefinedElsewhere : public juce::Component
{
public:
    DefinedElsewhere()
    {
        openGLContext.attachTo(*this);
    }

    ~DefinedElsewhere() override;

private:
    juce::OpenGLContext openGLContext;
};
