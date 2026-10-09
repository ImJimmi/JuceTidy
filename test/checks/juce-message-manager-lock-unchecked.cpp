#include <juce_mock.h>

class Worker : public juce::Thread
{
public:
    Worker()
        : juce::Thread("Worker")
    {
    }

    ~Worker() override
    {
        stopThread(1000);
    }

    void run() override
    {
        const juce::MessageManagerLock unchecked(this); // EXPECT_WARNING

        const juce::MessageManagerLock checked(juce::Thread::getCurrentThread());

        if (! checked.lockWasGained())
            return;

        const juce::MessageManagerLock blocking;
        const juce::MessageManagerLock alsoBlocking(nullptr);
    }
};
