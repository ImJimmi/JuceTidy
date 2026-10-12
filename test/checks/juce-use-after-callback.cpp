#include <juce_mock.h>

#include <functional>
#include <vector>

class Event
{
public:
    void trigger()
    {
        for (auto& callback : callbacks)
            callback();
    }

    void triggerWithoutSelfCallback()
    {
        const auto callback = onTrigger;
        onTrigger = nullptr;
        trigger();
        onTrigger = callback; // EXPECT_WARNING
    }

    void triggerSafely()
    {
        const auto callback = onTrigger;
        onTrigger = nullptr;
        juce::WeakReference<Event> weakThis{ this };
        trigger();

        if (weakThis != nullptr)
            weakThis->onTrigger = callback;
    }

    std::function<void()> onTrigger;
    std::vector<std::function<void()>> callbacks;
};

class Knob : public juce::Component
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void knobChanged() = 0;
    };

    void mouseUp(const juce::MouseEvent&) override
    {
        if (onClick != nullptr)
            onClick();

        repaint(); // EXPECT_WARNING
    }

    void notify()
    {
        listeners.call([](Listener& listener) {
            listener.knobChanged();
        });

        value = 0; // EXPECT_WARNING
    }

    void broadcast()
    {
        broadcaster.sendSynchronousChangeMessage();
        this->repaint(); // EXPECT_WARNING
    }

    void notifyLast()
    {
        value = 1;

        if (onClick != nullptr)
            onClick();
    }

    void notifyChecked()
    {
        const juce::Component::BailOutChecker checker{ this };

        if (onClick != nullptr)
            onClick();

        if (!checker.shouldBailOut())
            repaint();
    }

    void build()
    {
        const auto made = factory();
        (void) made;
        repaint();
    }

    void triggerAndRead()
    {
        const auto handled = notifyAll();
        value = handled ? 1 : 0; // EXPECT_WARNING
    }

    bool notifyAll()
    {
        if (onClick != nullptr)
            onClick();

        return true;
    }

    void localFunction()
    {
        const std::function<void()> update = [] {};
        update();
        repaint();
    }

    void deferred()
    {
        juce::MessageManager::callAsync([this] {
            if (onClick != nullptr)
                onClick();
        });

        repaint();
    }

    std::function<void()> onClick;
    std::function<int()> factory;

private:
    juce::ListenerList<Listener> listeners;
    juce::ChangeBroadcaster broadcaster;
    int value = 0;
};
