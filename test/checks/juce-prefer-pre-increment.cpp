#include <juce_mock.h>

#include <vector>

void loops(std::vector<int>& values)
{
    for (int i = 0; i < 10; i++) // EXPECT_WARNING
    {
    }

    for (auto it = values.begin(); it != values.end(); it++) // EXPECT_WARNING
    {
    }

    for (int i = 10, j = 0; i > 0; i--, j++) // EXPECT_WARNING
    {
    }
}

void statements(int count, bool condition)
{
    count++; // EXPECT_WARNING

    if (condition)
        count--; // EXPECT_WARNING
}

template <typename Iterator>
void templated(Iterator iterator)
{
    iterator++; // EXPECT_WARNING
}

template void templated(int);
template void templated(std::vector<int>::iterator);

int allowed(int count, int* pointer)
{
    ++count;
    --count;

    const auto previous = count++;
    pointer[count--] = 0;

    while (count--)
    {
    }

    return previous + count++;
}
