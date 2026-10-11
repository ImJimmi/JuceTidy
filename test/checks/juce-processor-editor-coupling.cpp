#include <juce_mock.h>

class Editor;

class Processor : public juce::AudioProcessor
{
public:
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
    }

    void loadPreset()
    {
        if (auto* editor = getActiveEditor()) // EXPECT_WARNING
            editor->repaint();

        juce::MessageManager::callAsync([this] {
            if (auto* editor = getActiveEditor()) // EXPECT_WARNING
                editor->repaint();
        });

        if (onPresetLoaded != nullptr)
            onPresetLoaded();
    }

    std::function<void()> onPresetLoaded;
    std::function<void()> onStateChanged;
    std::function<void()> onBypassChanged;
    Editor* editor = nullptr;
    int presetIndex = 0;
};

class Editor : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor& p)
        : juce::AudioProcessorEditor{ p }
        , processor{ p }
    {
        processor.onPresetLoaded = [this] { // EXPECT_WARNING
            repaint();
        };

        processor.onStateChanged = [this] {
            repaint();
        };

        processor.onBypassChanged = [] {};

        processor.editor = this; // EXPECT_WARNING

        processor.presetIndex = 0;
    }

    ~Editor() override
    {
        processor.onStateChanged = nullptr;
    }

private:
    Processor& processor;
};

class PointerEditor : public juce::AudioProcessorEditor
{
public:
    explicit PointerEditor(Processor* p)
        : juce::AudioProcessorEditor{ *p }
        , processor{ p }
    {
        processor->onPresetLoaded = [&] { // EXPECT_WARNING
            repaint();
        };
    }

private:
    Processor* processor;
};

class Host
{
public:
    void refresh(Processor& processor)
    {
        if (auto* editor = processor.getActiveEditor())
            editor->repaint();

        processor.onPresetLoaded = [this] {
            ++refreshCount;
        };
    }

private:
    int refreshCount = 0;
};
