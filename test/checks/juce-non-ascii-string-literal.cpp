#include <juce_mock.h>

void setText(const juce::String& text);

void literals()
{
    const juce::String degrees("90°"); // EXPECT_WARNING

    setText("Ünïcödé"); // EXPECT_WARNING

    setText("caf\xc3\xa9"); // EXPECT_WARNING

    const juce::String truncated("héllo", 2); // EXPECT_WARNING

    setText("plain ASCII");
    setText(juce::String::fromUTF8("90°"));
    setText(u8"90°");
    const juce::StringRef reference("90°");
    (void) degrees;
    (void) reference;
}
