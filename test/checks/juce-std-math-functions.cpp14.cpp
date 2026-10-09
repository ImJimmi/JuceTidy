// STD: c++14

namespace juce
{
template <typename Type>
constexpr Type jmax(Type a, Type b);

template <typename Type>
Type jlimit(Type lowerLimit, Type upperLimit, Type valueToConstrain) noexcept;
} // namespace juce

void beforeClamp(double d)
{
    (void) juce::jmax(d, 0.0); // EXPECT_WARNING

    (void) juce::jlimit(0.0, 1.0, d);
}
