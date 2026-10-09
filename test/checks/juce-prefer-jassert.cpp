#include <juce_mock.h>

#include <cassert>

void assertions(int value)
{
    assert(value > 0); // EXPECT_WARNING

    jassert (false); // EXPECT_WARNING

    jassert(0); // EXPECT_WARNING

    jassert (value > 0);
    jassert (false && value > 0);
    jassertfalse;
    static_assert(sizeof(int) == 4);
}
