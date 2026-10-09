#include <juce_mock.h>

void process();

class Endless : public juce::Thread
{
public:
    Endless()
        : Thread("Endless")
    {
    }

    void run() override
    {
        while (true) // EXPECT_WARNING
        {
            process();
            wait(100);
        }
    }
};

class Forever : public juce::Thread
{
public:
    Forever()
        : Thread("Forever")
    {
    }

    void run() override
    {
        for (;;) // EXPECT_WARNING
        {
            process();
        }

        do // EXPECT_WARNING
        {
            process();
        } while (1);
    }
};

class Checking : public juce::Thread
{
public:
    Checking()
        : Thread("Checking")
    {
    }

    void run() override
    {
        while (! threadShouldExit())
            process();

        for (;;)
        {
            if (juce::Thread::currentThreadShouldExit())
                return;

            process();
        }
    }
};

class NotAThread
{
public:
    void run()
    {
        while (true)
            process();
    }
};
