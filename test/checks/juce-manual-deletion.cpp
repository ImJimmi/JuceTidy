#include <juce_mock.h>

struct Thing
{
};

void deletion(Thing* thing, Thing* things)
{
    juce::deleteAndZero(thing); // EXPECT_WARNING

    delete thing;
    delete[] things;
}

class Editor : public juce::Component
{
public:
    ~Editor() override
    {
        deleteAllChildren(); // EXPECT_WARNING
    }
};
