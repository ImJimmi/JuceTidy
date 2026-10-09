// OPTION: PreferJuceFunctions=true

#include <juce_mock.h>

#include <algorithm>
#include <math.h>

void standardFunctions(float f, double d, int i)
{
    [[maybe_unused]] const auto a = std::max(f, 1.0f); // EXPECT_WARNING

    [[maybe_unused]] const auto b = std::min({ i, 2, 3 }); // EXPECT_WARNING

    [[maybe_unused]] const auto c = std::clamp(d, 0.0, 1.0); // EXPECT_WARNING

    [[maybe_unused]] const auto e = std::hypot(f, f); // EXPECT_WARNING

    [[maybe_unused]] const auto g = std::isfinite(d); // EXPECT_WARNING

    [[maybe_unused]] const auto h = hypotf(f, f); // EXPECT_WARNING

    [[maybe_unused]] const auto j = sqrtf(f); // EXPECT_WARNING

    [[maybe_unused]] const auto k = std::max<int>(i, 0); // EXPECT_WARNING

    using std::max;
    [[maybe_unused]] const auto l = max(i, 0); // EXPECT_WARNING
}

void allowed(float f, double d, int i)
{
    [[maybe_unused]] const auto a = juce::jmax(f, 1.0f);
    [[maybe_unused]] const auto b = juce::jmin(i, 2, 3);
    [[maybe_unused]] const auto c = juce::jlimit(0.0, 1.0, d);
    [[maybe_unused]] const auto e = juce::juce_hypot(f, f);
    [[maybe_unused]] const auto g = juce::juce_isfinite(d);
    [[maybe_unused]] const auto h = std::sqrt(f);
    [[maybe_unused]] const auto j = std::max(i, 0, std::greater<>());
    [[maybe_unused]] const auto k = std::min({ i, 1, 2, 3, 4 });
    [[maybe_unused]] const auto l = std::min({ i, 1 }, std::greater<>());
    [[maybe_unused]] const auto m = std::clamp(i, 0, 1, std::less<>());
    [[maybe_unused]] const auto n = std::hypot(d, d, d);
}
