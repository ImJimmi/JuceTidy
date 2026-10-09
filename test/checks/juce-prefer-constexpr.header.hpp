#include <juce_mock.h>

const int numChannels = 2; // EXPECT_WARNING

const juce::Colour background{ 0xff000000 }; // EXPECT_WARNING

static const float defaultGain = 0.5f; // EXPECT_WARNING

inline const int alreadyInline = 3; // EXPECT_WARNING

namespace
{
const int hidden = 4; // EXPECT_WARNING
}

struct Settings
{
    static const int maxVoices = 16; // EXPECT_WARNING
};

extern const int declaredElsewhere;
