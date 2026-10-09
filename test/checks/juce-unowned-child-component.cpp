#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    Editor()
    {
        addAndMakeVisible(new juce::Label()); // EXPECT_WARNING

        addChildComponent(new juce::TextButton("Button")); // EXPECT_WARNING

        addAndMakeVisible(label);
        addAndMakeVisible(owned.get());
        addAndMakeVisible(buttons.add(new juce::TextButton()));
    }

private:
    juce::Label label;
    std::unique_ptr<juce::Component> owned = std::make_unique<juce::Label>();
    juce::OwnedArray<juce::TextButton> buttons;
};

void windows(juce::DocumentWindow& window)
{
    window.setContentNonOwned(new juce::Label(), true); // EXPECT_WARNING

    window.setContentOwned(new juce::Label(), true);
}
