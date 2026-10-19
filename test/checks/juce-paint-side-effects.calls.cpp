#include <juce_mock.h>

#include <memory>
#include <vector>

class Shadow : public juce::Component
{
public:
    explicit Shadow(juce::Component& target)
    {
        if (auto* parent = target.getParentComponent())
            parent->addChildComponent(this, 0); // EXPECT_WARNING
    }
};

class Highlight : public juce::Component
{
public:
    explicit Highlight(juce::Component& target)
    {
        if (auto* parent = target.getParentComponent())
            parent->addAndMakeVisible(this); // EXPECT_WARNING
    }
};

class Overlay : public juce::Component
{
public:
    explicit Overlay(juce::Component& target)
        : juce::Component{}
        , caption{}
    {
        target.addAndMakeVisible(this); // EXPECT_WARNING
    }

private:
    juce::Label caption;
};

// Defined in another file, so its constructor can't be checked.
class Glow : public juce::Component
{
public:
    explicit Glow(juce::Component& target);
};

class Styler
{
public:
    void updateShadow(juce::Component& component)
    {
        if (shadow == nullptr)
            shadow = std::make_unique<Shadow>(component); // EXPECT_WARNING
    }

    void updateHighlight(juce::Component& component)
    {
        if (highlight == nullptr)
            highlight = std::make_shared<Highlight>(component); // EXPECT_WARNING
    }

    void updateGlow(juce::Component& component)
    {
        if (glow == nullptr)
            glow = std::make_unique<Glow>(component); // EXPECT_WARNING
    }

    template <typename ComponentType>
    void resize(ComponentType& component)
    {
        component.setBounds(0, 0, 10, 10); // EXPECT_WARNING
    }

private:
    std::unique_ptr<Shadow> shadow;
    std::shared_ptr<Highlight> highlight;
    std::unique_ptr<Glow> glow;
};

class Meter : public juce::Component
{
public:
    void paint(juce::Graphics&) override
    {
        updateLabel();
        styler.updateShadow(*this);
        styler.updateHighlight(*this);
        styler.updateGlow(*this);
        styler.resize(label);
        styler.resize(other);
        Overlay overlay{ *this }; // EXPECT_WARNING

        [this] {
            other.setVisible(true); // EXPECT_WARNING
        }();

        juce::MessageManager::callAsync([this] {
            addAndMakeVisible(label);
            glows.push_back(std::make_unique<Glow>(*this));
        });
    }

    void resized() override
    {
        layOut();
    }

private:
    void updateLabel()
    {
        label.setBounds(0, 0, 10, 10); // EXPECT_WARNING
        repaint(); // EXPECT_WARNING
    }

    void layOut()
    {
        label.setBounds(0, 0, 10, 10);
        addAndMakeVisible(label);
        glows.push_back(std::make_unique<Glow>(*this));
    }

    juce::Label label;
    juce::TextButton other;
    std::vector<std::unique_ptr<Glow>> glows;
    Styler styler;
};

class Dial : public juce::Component
{
public:
    void paint(juce::Graphics&) override
    {
        styler.updateShadow(*this);
    }

private:
    Styler styler;
};
