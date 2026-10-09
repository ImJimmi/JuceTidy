#include <juce_mock.h>

const int numChannels = 2; // EXPECT_WARNING

static const float defaultGain = 0.5f; // EXPECT_WARNING

const char* const name = "Plugin"; // EXPECT_WARNING

const juce::Colour background{ 0xff000000 }; // EXPECT_WARNING

struct Settings
{
    static const int maxVoices = 16; // EXPECT_WARNING
};

int compute(int input);

void locals(int input)
{
    const auto half = 0.5f; // EXPECT_WARNING

    const int doubled = numChannels * 2; // EXPECT_WARNING

    const int first = 1, second = compute(input); // EXPECT_WARNING

    static const int alreadyStatic = 3; // EXPECT_WARNING

    if (const int condition = 1) // EXPECT_WARNING
        (void) condition;

    const int captured = 4; // EXPECT_WARNING
    const auto capture = [captured] { return captured; };

    const auto withLocal = []
    {
        const int insideLambda = 5; // EXPECT_WARNING
        return insideLambda;
    };

    const auto result = compute(input);
    const auto copy = input;
    const juce::String text = "text";
    const auto lambda = [] { return 1; };
    constexpr auto alreadyConstexpr = 1;
    int mutableValue = 1;
    const auto& reference = numChannels;
    const auto [a, b] = std::pair{ 1, 2 };

    (void) half;
    (void) doubled;
    (void) first;
    (void) second;
    (void) alreadyStatic;
    (void) capture;
    (void) withLocal;
    (void) result;
    (void) copy;
    (void) lambda;
    (void) alreadyConstexpr;
    (void) mutableValue;
    (void) reference;
    (void) a;
    (void) b;
}

constexpr int constexprFunction()
{
    const int insideConstexpr = 6; // EXPECT_WARNING
    return insideConstexpr;
}

template <typename Type>
void templated()
{
    const Type value = Type(1);
    (void) value;
}

extern const int declaredElsewhere;
