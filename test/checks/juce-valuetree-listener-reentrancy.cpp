#include <juce_mock.h>

class Watcher : private juce::ValueTree::Listener
{
private:
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier&) override
    {
        tree.setProperty("dirty", 1, nullptr); // EXPECT_WARNING
        tree.getParent().removeChild(tree, nullptr); // EXPECT_WARNING

        const auto& value = tree.getProperty("gain");
        (void) value;

        other.setProperty("dirty", 1, nullptr);

        juce::MessageManager::callAsync([tree]() mutable {
            tree.setProperty("dirty", 0, nullptr);
        });
    }

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override
    {
        child.removeAllChildren(nullptr); // EXPECT_WARNING
        parent.getChildWithName("Bands").appendChild(juce::ValueTree{ "Band" }, nullptr); // EXPECT_WARNING
    }

    juce::ValueTree other;
};

void update(juce::ValueTree& tree)
{
    tree.setProperty("dirty", 1, nullptr);
}
