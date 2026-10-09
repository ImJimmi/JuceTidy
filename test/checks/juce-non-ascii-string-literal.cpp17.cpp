// STD: c++17

#include <juce_mock.h>

void setText(const juce::String& text);

void literals()
{
    setText(u8"90°"); // EXPECT_WARNING

    const juce::String truncated(u8"héllo", 2); // EXPECT_WARNING

    setText(u8"plain ASCII");
    setText(juce::String::fromUTF8(u8"90°"));
}
