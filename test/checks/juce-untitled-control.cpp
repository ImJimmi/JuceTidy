#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    Editor()
        : namedButton{ "Play" }
        , namedSlider{ "Gain" }
    {
        titled.setTitle("Gain");
        titledOwned->setTitle("Mix");
        textButton.setButtonText("Stop");
        ownedButton = std::make_unique<juce::TextButton>("Record");

        for (auto* slider : { &first, &second })
            slider->setTitle("Band");

        setUpComboBox(helperTitled);

        const auto setUpEditor = [](juce::TextEditor& editor) {
            editor.setTitle("Name");
        };
        setUpEditor(lambdaTitled);

        addAndMakeVisible(untitled);
        addAndMakeVisible(namedSlider);
        untitledComboBox.setName("Mode");
    }

private:
    void setUpComboBox(juce::ComboBox& comboBox)
    {
        comboBox.setTitle("Mode");
    }

    juce::Slider titled;
    std::unique_ptr<juce::Slider> titledOwned = std::make_unique<juce::Slider>();
    juce::Slider first;
    juce::Slider second;
    juce::ComboBox helperTitled;
    juce::TextEditor lambdaTitled;
    juce::TextButton textButton;
    juce::TextButton namedButton;
    juce::ToggleButton inClassNamedButton{ "Bypass" };
    juce::ImageButton imageButton{ "Logo" };
    std::unique_ptr<juce::TextButton> ownedButton;
    std::unique_ptr<juce::TextButton> inClassOwnedButton = std::make_unique<juce::TextButton>("Load");
    juce::Label label;

    juce::Slider untitled; // EXPECT_WARNING
    juce::Slider namedSlider; // EXPECT_WARNING
    juce::ComboBox untitledComboBox; // EXPECT_WARNING
    juce::TextEditor untitledEditor; // EXPECT_WARNING
    juce::TextButton untitledButton; // EXPECT_WARNING
    juce::ToggleButton untitledToggle; // EXPECT_WARNING
    std::unique_ptr<juce::Slider> untitledOwned = std::make_unique<juce::Slider>(); // EXPECT_WARNING
};

class Knob : public juce::Slider
{
public:
    Knob()
    {
        setTitle("Knob");
    }
};

class LargeKnob : public Knob
{
};

class PlainKnob : public juce::Slider
{
};

class NamedKnob : public juce::Slider
{
public:
    NamedKnob()
        : juce::Slider{ "Gain" }
    {
    }
};

class PlayButton : public juce::TextButton
{
public:
    PlayButton()
        : juce::TextButton{ "Play" }
    {
    }
};

class IconButton : public juce::ImageButton
{
};

class KnobDefinedElsewhere : public juce::Slider
{
public:
    KnobDefinedElsewhere();
};

class CustomControls : public juce::Component
{
public:
    CustomControls()
    {
        titledPlainKnob.setTitle("Mix");
        namedIconButton.setButtonText("Settings");
    }

private:
    Knob knob;
    LargeKnob largeKnob;
    PlainKnob titledPlainKnob;
    PlayButton playButton;
    IconButton namedIconButton;
    KnobDefinedElsewhere knobDefinedElsewhere;

    PlainKnob plainKnob; // EXPECT_WARNING
    std::unique_ptr<PlainKnob> ownedPlainKnob = std::make_unique<PlainKnob>(); // EXPECT_WARNING
    NamedKnob namedKnob; // EXPECT_WARNING
    IconButton iconButton; // EXPECT_WARNING
};

class DefinedElsewhere : public juce::Component
{
public:
    DefinedElsewhere();

private:
    juce::Slider slider;
};
