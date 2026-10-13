#include <juce_mock.h>

namespace ids
{
const juce::Identifier gain("gain");
}

class State
{
public:
    State()
        : tree("state")
        , nameId("name")
    {
        tree.setProperty("gain", 0.0f, nullptr);

        button.onClick = [this] {
            tree.setProperty("gain", 1.0f, nullptr); // EXPECT_WARNING
        };
    }

    float getGain() const
    {
        tree.getProperty("gain"); // EXPECT_WARNING

        const juce::Identifier local("mix"); // EXPECT_WARNING

        static const juce::Identifier cached("pan");
        tree.getProperty(cached);
        tree.getProperty(ids::gain);
        tree.getProperty(typeId);
        (void) local;
        return 0.0f;
    }

private:
    juce::ValueTree tree;
    juce::TextButton button;
    const juce::Identifier nameId;
    const juce::Identifier typeId{ "type" };
    static inline const juce::Identifier staticId{ "static" };
};
