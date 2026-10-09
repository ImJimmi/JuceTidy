#include <juce_mock.h>

class Processor : public juce::AudioProcessor
{
public:
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "state", {} };
    juce::ChangeBroadcaster broadcaster;
};

class MissingRemoval : public juce::Component,
                       private juce::AudioProcessorValueTreeState::Listener,
                       private juce::ChangeListener
{
public:
    explicit MissingRemoval(Processor& p)
        : processor(p)
    {
        processor.apvts.addParameterListener("gain", this); // EXPECT_WARNING

        processor.broadcaster.addChangeListener(this); // EXPECT_WARNING
    }

    ~MissingRemoval() override
    {
        processor.broadcaster.sendChangeMessage();
    }

private:
    void parameterChanged(const juce::String&, float) override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    Processor& processor;
};

class NoDestructor : public juce::Component,
                     private juce::Slider::Listener
{
public:
    explicit NoDestructor(juce::Slider* external)
    {
        external->addListener(this); // EXPECT_WARNING
    }

private:
    void sliderValueChanged(juce::Slider*) override;
};

class RemovesInDestructor : private juce::ChangeListener
{
public:
    explicit RemovesInDestructor(juce::ChangeBroadcaster& source)
        : broadcaster(source)
    {
        broadcaster.addChangeListener(this);
    }

    ~RemovesInDestructor() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    juce::ChangeBroadcaster& broadcaster;
};

RemovesInDestructor::~RemovesInDestructor()
{
    broadcaster.removeChangeListener(this);
}

class OwnsBroadcasters : public juce::Component,
                         private juce::Slider::Listener,
                         private juce::Button::Listener
{
public:
    OwnsBroadcasters()
    {
        slider.addListener(this);
        owned->addListener(this);
        buttons[0]->addListener(this);
        addMouseListener(this, true);

        for (auto* button : { &first, &second })
            button->addListener(this);

        for (auto* button : buttons)
            button->addListener(this);

        for (auto* button : getButtons())
            button->addListener(this);

        getButtons()[0]->addListener(this);
    }

private:
    void sliderValueChanged(juce::Slider*) override;
    void buttonClicked(juce::Button*) override;

    juce::Slider slider;
    std::unique_ptr<juce::Slider> owned;
    juce::TextButton first;
    juce::TextButton second;
    std::vector<juce::TextButton*> buttons;

    std::vector<juce::TextButton*> getButtons();
};

class ExternalSingletons : private juce::ChangeListener
{
public:
    ExternalSingletons()
    {
        getBroadcaster().addChangeListener(this); // EXPECT_WARNING

        globalBroadcaster.addChangeListener(this); // EXPECT_WARNING
    }

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    static juce::ChangeBroadcaster& getBroadcaster();
    static inline juce::ChangeBroadcaster globalBroadcaster;
};

class DefinedElsewhere : private juce::ChangeListener
{
public:
    explicit DefinedElsewhere(juce::ChangeBroadcaster& source)
    {
        source.addChangeListener(this);
    }

    ~DefinedElsewhere() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
};

class AudioEngine : public juce::AudioIODeviceCallback,
                    private juce::TimeSliceClient
{
public:
    AudioEngine(juce::AudioDeviceManager& manager, juce::TimeSliceThread& thread)
        : deviceManager(manager)
        , backgroundThread(thread)
    {
        deviceManager.addAudioCallback(this); // EXPECT_WARNING
        backgroundThread.addTimeSliceClient(this);
    }

    ~AudioEngine() override
    {
        backgroundThread.removeTimeSliceClient(this);
    }

private:
    int useTimeSlice() override;

    juce::AudioDeviceManager& deviceManager;
    juce::TimeSliceThread& backgroundThread;
};
