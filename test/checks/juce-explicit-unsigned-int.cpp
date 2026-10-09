#include <juce_mock.h>

unsigned count = 0; // EXPECT_WARNING

void parameters(const unsigned value, unsigned* pointer); // EXPECT_WARNING

auto cast(int value)
{
    return static_cast<unsigned>(value); // EXPECT_WARNING
}

unsigned int explicitInt = 0;
unsigned char byte = 0;
unsigned short shortValue = 0;
unsigned long longValue = 0;
juce::uint32 fixedWidth = 0;
