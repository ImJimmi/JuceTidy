#include <juce_mock.h>

bool compare(float a, float b, double c, int i)
{
    if (a == b) // EXPECT_WARNING
        return true;

    if (a != b) // EXPECT_WARNING
        return false;

    if (a == c) // EXPECT_WARNING
        return true;

    if (c == 0.0) // EXPECT_WARNING
        return true;

    if (a != a)
        return false;

    if (i == 3)
        return true;

    return a < b;
}
