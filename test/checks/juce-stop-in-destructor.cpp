#include <juce_mock.h>

class Worker : public juce::Thread // EXPECT_WARNING
{
public:
    Worker()
        : juce::Thread("Worker")
    {
        startThread();
    }

    void run() override;
};

class StoppedWorker : public juce::Thread
{
public:
    StoppedWorker()
        : juce::Thread("Worker")
    {
    }

    ~StoppedWorker() override
    {
        stopThread(1000);
    }

    void run() override;
};

class IndirectlyStoppedWorker : public juce::Thread
{
public:
    IndirectlyStoppedWorker()
        : juce::Thread("Worker")
    {
    }

    ~IndirectlyStoppedWorker() override
    {
        shutdown();
    }

    void run() override;

private:
    void shutdown()
    {
        signalThreadShouldExit();
        stopThread(1000);
    }
};

class Meter : public juce::HighResolutionTimer // EXPECT_WARNING
{
public:
    ~Meter() override
    {
    }

    void hiResTimerCallback() override;
};

class StoppedMeter : public juce::HighResolutionTimer
{
public:
    ~StoppedMeter() override
    {
        stopTimer();
    }

    void hiResTimerCallback() override;
};

class AbstractWorker : public juce::Thread
{
public:
    using juce::Thread::Thread;
};

class RegularTimer : public juce::Timer
{
    void timerCallback() override;
};
