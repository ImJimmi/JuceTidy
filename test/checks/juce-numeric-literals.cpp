#include <juce_mock.h>

void floatingPoint()
{
    [[maybe_unused]] const auto a = 1.; // EXPECT_WARNING

    [[maybe_unused]] const auto b = .5f; // EXPECT_WARNING

    [[maybe_unused]] const auto c = 2.f; // EXPECT_WARNING

    [[maybe_unused]] const auto d = 1.e3; // EXPECT_WARNING

    [[maybe_unused]] const auto allowed = 0.0 + 0.5f + 1e3 + 1'000.0 + 0x1.8p3;
}

void hexadecimal()
{
    [[maybe_unused]] const auto a = 0xFF00FF; // EXPECT_WARNING

    [[maybe_unused]] const auto b = 0XabcdefU; // EXPECT_WARNING

    [[maybe_unused]] const auto allowed = 0xff00ff + 0xabcdefu + 0xFULL - 0xfULL + 255; // EXPECT_WARNING
}

#define HALF .5
[[maybe_unused]] const auto fromMacro = HALF;
