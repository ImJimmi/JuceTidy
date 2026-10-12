#include <juce_mock.h>

class RegisteredTest : public juce::UnitTest
{
public:
    RegisteredTest()
        : juce::UnitTest{ "Registered", "Tests" }
    {
    }

    void runTest() override
    {
    }
};

static RegisteredTest registeredTest;

class ForgottenTest : public juce::UnitTest // EXPECT_WARNING
{
public:
    ForgottenTest()
        : juce::UnitTest{ "Forgotten", "Tests" }
    {
    }

    void runTest() override
    {
    }
};

class AbstractTest : public juce::UnitTest
{
public:
    using juce::UnitTest::UnitTest;
};

class LocalTest : public juce::UnitTest
{
public:
    LocalTest()
        : juce::UnitTest{ "Local" }
    {
    }

    void runTest() override
    {
    }
};

class SharedTest : public juce::UnitTest
{
public:
    using juce::UnitTest::UnitTest;

    void runTest() override
    {
    }
};

class DerivedTest : public SharedTest
{
public:
    DerivedTest()
        : SharedTest{ "Derived" }
    {
    }
};

static DerivedTest derivedTest;

template <typename SampleType>
class TemplatedTest : public juce::UnitTest
{
public:
    TemplatedTest()
        : juce::UnitTest{ "Templated" }
    {
    }

    void runTest() override
    {
    }
};

void runLocalTests()
{
    LocalTest test;
    (void) test;
}
