#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    void openFile()
    {
        juce::FileChooser chooser("Open");
        chooser.launchAsync(0, [](const juce::FileChooser&) {}); // EXPECT_WARNING
    }

    void openFileCorrectly()
    {
        memberChooser = std::make_unique<juce::FileChooser>("Open");
        memberChooser->launchAsync(0, [](const juce::FileChooser&) {});

        auto& chooser = *memberChooser;
        chooser.launchAsync(0, [](const juce::FileChooser&) {});
    }

private:
    std::unique_ptr<juce::FileChooser> memberChooser;
};
