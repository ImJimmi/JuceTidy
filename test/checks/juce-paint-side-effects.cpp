#include <juce_mock.h>

class Meter : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff000000));
        repaint(); // EXPECT_WARNING
        label.setBounds(0, 0, 10, 10); // EXPECT_WARNING
        addAndMakeVisible(label); // EXPECT_WARNING
        addChildComponent(label); // EXPECT_WARNING
        removeChildComponent(&label); // EXPECT_WARNING
        removeAllChildren(); // EXPECT_WARNING
        deleteAllChildren(); // EXPECT_WARNING

        juce::Label temporary; // EXPECT_WARNING
        auto* added = new juce::Label(); // EXPECT_WARNING
        addAndMakeVisible(added); // EXPECT_WARNING
    }

    void paintOverChildren(juce::Graphics&) override
    {
        setVisible(false); // EXPECT_WARNING
        label.toFront(false); // EXPECT_WARNING
        label.toBack(); // EXPECT_WARNING
        label.toBehind(&other); // EXPECT_WARNING
    }

    void resized() override
    {
        label.setBounds(0, 0, 10, 10);
        repaint();
        addChildComponent(label);
        label.toFront(false);

        juce::Label temporary;
    }

private:
    juce::Label label;
    juce::Label other;
};
