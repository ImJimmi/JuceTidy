// OPTION: LayoutFunctionName=performLayout
#include <juce_mock.h>

class Editor : public juce::Component
{
public:
    void refresh()
    {
        resized(); // EXPECT_WARNING
    }

    void resized() override
    {
        performLayout();
    }

private:
    void performLayout() {}
};
