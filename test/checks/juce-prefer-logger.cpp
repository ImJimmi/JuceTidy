// STD: c++23
#include <juce_mock.h>

#include <cstdio>
#include <iostream>
#include <print>

void report(int value, std::FILE* file)
{
    std::cout << "value: " << value << '\n'; // EXPECT_WARNING
    std::cerr << "error\n"; // EXPECT_WARNING

    std::print("value: {}\n", value); // EXPECT_WARNING
    std::println("value: {}", value); // EXPECT_WARNING
    std::print(stderr, "error\n"); // EXPECT_WARNING
    std::vprint_unicode("value: {}\n", std::make_format_args(value)); // EXPECT_WARNING

    std::printf("%d\n", value); // EXPECT_WARNING
    std::puts("done"); // EXPECT_WARNING
    std::putchar('\n'); // EXPECT_WARNING
    std::fprintf(stdout, "%d\n", value); // EXPECT_WARNING
    std::fprintf(stderr, "error\n"); // EXPECT_WARNING
    std::fputs("done\n", stdout); // EXPECT_WARNING
    std::fputc('\n', stderr); // EXPECT_WARNING

    juce::Logger::writeToLog("value");
    std::println(file, "saved {}", value);
    std::fprintf(file, "%d\n", value);
    std::fputs("saved\n", file);
}
