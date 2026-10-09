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
    }

    void paintOverChildren(juce::Graphics&) override
    {
        setVisible(false); // EXPECT_WARNING
    }

    void resized() override
    {
        label.setBounds(0, 0, 10, 10);
        repaint();
    }

private:
    juce::Label label;
};
