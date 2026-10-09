#include <juce_mock.h>

void files()
{
    const juce::File settings("Settings/preset.xml"); // EXPECT_WARNING

    const juce::File unix("/Users/shared/preset.xml");
    const juce::File home("~/Music/preset.xml");
    const juce::File windows("C:\\Presets\\preset.xml");
    const juce::File network("\\\\server\\presets");
    const juce::File empty("");
    const auto child = juce::File::getCurrentWorkingDirectory().getChildFile("Settings/preset.xml");
}
