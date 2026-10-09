#include <juce_mock.h>

class Correct : public juce::Component
{
public:
    Correct();
    void paint(juce::Graphics&) override;

protected:
    void helper();

private:
    struct Impl;
    int value = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Correct)
};

class PrivateFirst
{
    int value = 0;

public: // EXPECT_WARNING
    int getValue() const;
};

class ProtectedAfterPrivate
{
public:
    void a();

private:
    void b();

protected: // EXPECT_WARNING
    void c();
    void d();

public: // EXPECT_WARNING
    void e();
};

struct StructWithPrivateFirst
{
private:
    int hidden = 0;

public: // EXPECT_WARNING
    int visible = 0;
};

class TypesMayComeFirst
{
    using Base = juce::Component;
    struct Detail
    {
    };

public:
    void a();

private:
    void b();
};

template <typename Type>
class Templated
{
private:
    Type value;

public: // EXPECT_WARNING
    Type get() const;
};

Templated<int> instantiation;
