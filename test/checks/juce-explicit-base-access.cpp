#include <juce_mock.h>

class Implicit : juce::Component // EXPECT_WARNING
{
};

struct ImplicitStruct : juce::Component, // EXPECT_WARNING
                        private juce::Timer
{
    void timerCallback() override {}
};

class Virtual : virtual juce::ChangeBroadcaster // EXPECT_WARNING
{
};

class Explicit : public juce::Component,
                 private juce::Timer
{
    void timerCallback() override {}
};

template <typename Base>
struct Templated : Base // EXPECT_WARNING
{
};

Templated<juce::Component> instantiation;
