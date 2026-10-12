#include <juce_mock.h>

#include <vector>

class MathsTest : public juce::UnitTest
{
public:
    MathsTest()
        : juce::UnitTest{ "Maths" }
    {
    }

    void runTest() override
    {
        beginTest("comparisons");

        const auto answer = 42;
        const auto other = 7;
        const std::vector<int> values{ 1, 2, 3 };
        const juce::String name{ "test" };

        expect(answer == other); // EXPECT_WARNING
        expect(answer != other, "should differ"); // EXPECT_WARNING
        expect(answer < other); // EXPECT_WARNING
        expect((answer >= other)); // EXPECT_WARNING
        expect(values.size() == 3); // EXPECT_WARNING
        expect(name == "test"); // EXPECT_WARNING
        expect(name == name); // EXPECT_WARNING
        expect(0.5f == 0.5f); // EXPECT_WARNING

        expectEquals(0.5, 0.5); // EXPECT_WARNING
        expectEquals(answer, other);
        expectWithinAbsoluteError(0.5, 0.5, 0.001);
        expect(values.empty());
        expect(!values.empty());
    }
};

static MathsTest mathsTest;
