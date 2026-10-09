#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    Editor()
    {
        addAndMakeVisible(added);
        addChildComponent(*owned);

        for (auto* button : { &first, &second })
            addAndMakeVisible(button);

        setUpLabel(helperAdded);
    }

    void resized() override
    {
        added.setBounds(0, 0, 10, 10);
        owned->setBounds(0, 0, 10, 10);
        forgotten.setBounds(0, 0, 10, 10);
        forgottenOwned->setBounds(0, 0, 10, 10);
        first.setBounds(0, 0, 10, 10);
        second.setBounds(0, 0, 10, 10);
        helperAdded.setBounds(0, 0, 10, 10);

        juce::FlexBox box;
        box.items.add(juce::FlexItem(laidOut));
        box.performLayout(getLocalBounds());
    }

private:
    void setUpLabel(juce::Label& label)
    {
        addAndMakeVisible(label);
    }

    juce::Slider added;
    std::unique_ptr<juce::Slider> owned = std::make_unique<juce::Slider>();
    juce::Slider forgotten; // EXPECT_WARNING
    std::unique_ptr<juce::Slider> forgottenOwned = std::make_unique<juce::Slider>(); // EXPECT_WARNING
    juce::TextButton first;
    juce::TextButton second;
    juce::Label helperAdded;
    juce::Label laidOut; // EXPECT_WARNING
    juce::Label neverPositioned; // EXPECT_WARNING
    juce::TooltipWindow tooltipWindow{this};
    std::unique_ptr<juce::DocumentWindow> window;
};

class DefinedElsewhere : public juce::Component
{
public:
    DefinedElsewhere();
    void resized() override
    {
        label.setBounds(0, 0, 10, 10);
    }

private:
    juce::Label label;
};
