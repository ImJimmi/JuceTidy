#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    void update()
    {
        juce::MessageManager::callAsync([this] { repaint(); }); // EXPECT_WARNING

        juce::Timer::callAfterDelay(100, [&] { repaint(); }); // EXPECT_WARNING

        juce::MessageManager::callAsync([=] { repaint(); }); // EXPECT_WARNING

        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<Editor>(this)] {
            if (safeThis != nullptr)
                safeThis->repaint();
        });

        juce::MessageManager::callAsync([this, safeThis = juce::Component::SafePointer<Editor>(this)] {
            if (safeThis != nullptr)
                repaint();
        });

        juce::MessageManager::callAsync([] {});

        auto callback = [this] { repaint(); };
        callback();
    }
};
