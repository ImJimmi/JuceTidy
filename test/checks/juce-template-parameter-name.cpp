#include <juce_mock.h>

template <typename T> // EXPECT_WARNING
struct Wrapper
{
    T value;
};

template <typename ValueType, int N, template <typename> class C> // EXPECT_WARNING
void function(C<ValueType>& container);

template <typename SampleType, int numChannels>
struct Descriptive
{
};

template <typename>
struct Unnamed
{
};

void generic(auto value);
auto lambda = [](auto value) { return value; };
