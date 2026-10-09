#include <juce_mock.h>

#include <algorithm>
#include <math.h>
#include <stdlib.h>

void cFunctions(float f, double d, int i)
{
    [[maybe_unused]] const auto a = sqrtf(f); // EXPECT_WARNING

    [[maybe_unused]] const auto b = fabs(d); // EXPECT_WARNING

    [[maybe_unused]] const auto c = powf(f, 2.0f); // EXPECT_WARNING

    [[maybe_unused]] const auto e = sinf(d); // EXPECT_WARNING

    [[maybe_unused]] const auto g = std::fabs(d); // EXPECT_WARNING

    [[maybe_unused]] const auto h = ::sqrt(d); // EXPECT_WARNING

    [[maybe_unused]] const auto j = abs(i); // EXPECT_WARNING
}

void juceFunctions(float f, double d, int i)
{
    [[maybe_unused]] const auto a = juce::jmax(f, 1.0f); // EXPECT_WARNING

    [[maybe_unused]] const auto b = juce::jmin(i, 2, 3); // EXPECT_WARNING

    [[maybe_unused]] const auto c = juce::jlimit(0.0, 1.0, d); // EXPECT_WARNING

    [[maybe_unused]] const auto e = juce::juce_hypot(f, f); // EXPECT_WARNING

    [[maybe_unused]] const auto g = juce::juce_isfinite(d); // EXPECT_WARNING

    [[maybe_unused]] const auto h = juce::jmax<int>(i, 0); // EXPECT_WARNING

    [[maybe_unused]] const auto j = juce::juce_hypot(i, i); // EXPECT_WARNING

    using namespace juce;
    [[maybe_unused]] const auto k = jmin(f, 0.0f, 1.0f, 2.0f); // EXPECT_WARNING
}

namespace custom
{
float sqrt(float);
}

void allowed(float f, double d, int i)
{
    [[maybe_unused]] const auto a = std::sqrt(f);
    [[maybe_unused]] const auto b = std::abs(d);
    [[maybe_unused]] const auto c = std::pow(f, 2.0f);
    [[maybe_unused]] const auto e = std::abs(i);
    [[maybe_unused]] const auto g = custom::sqrt(f);
    [[maybe_unused]] const auto j = std::max(f, 1.0f);
    [[maybe_unused]] const auto k = std::min({ i, 2, 3 });
    [[maybe_unused]] const auto l = std::clamp(d, 0.0, 1.0);
    [[maybe_unused]] const auto m = std::hypot(f, f);
    [[maybe_unused]] const auto n = std::isfinite(d);

    using std::sin;
    [[maybe_unused]] const auto h = sin(d);
}
