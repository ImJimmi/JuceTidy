#include <juce_mock.h>

class Watcher : private juce::ValueTree::Listener,
                private juce::Value::Listener
{
public:
    explicit Watcher(juce::AudioProcessorValueTreeState& apvts)
    {
        auto copy = apvts.state;
        copy.addListener(this); // EXPECT_WARNING

        apvts.state.getChildWithName("child").addListener(this); // EXPECT_WARNING

        juce::Value value;
        value.addListener(this); // EXPECT_WARNING

        tree = apvts.state;
        tree.addListener(this);
        apvts.state.addListener(this);

        auto& reference = apvts.state;
        reference.addListener(this);
    }

    void watch(juce::ValueTree byValue)
    {
        byValue.addListener(this); // EXPECT_WARNING
    }

private:
    void valueChanged(juce::Value&) override;

    juce::ValueTree tree;
};
