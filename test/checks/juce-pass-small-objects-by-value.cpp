#include <juce_mock.h>

void setColour(const juce::Colour& colour); // EXPECT_WARNING

void setBounds(const juce::Rectangle<int>& bounds, const juce::Point<float> &position, const juce::Identifier&); // EXPECT_WARNING

using namespace juce;

void usingNamespace(const StringRef& text); // EXPECT_WARNING

template <typename ValueType>
void templated(const Range<ValueType>& range); // EXPECT_WARNING

struct Base
{
    virtual ~Base() = default;
    virtual void draw(const Colour& colour); // EXPECT_WARNING
};

struct Derived : public Base
{
    void draw(const Colour& colour) override;
};

void allowed(Colour colour, const String& text, juce::Point<int> point, Colour& output, const juce::MouseEvent& event);
