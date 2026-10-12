#include <juce_mock.h>

//==============================================================================
class Meter
{
public:
    //==============================================================================
    void reset();

    //--------------------------------------------------------------------------
    // EXPECT_WARNING_ABOVE
    void process();

    //======================================
    // EXPECT_WARNING_ABOVE
    void prepare();

    //================================================================================
    // EXPECT_WARNING_ABOVE
    void release();

    //==========================================================================
    // EXPECT_WARNING_ABOVE
    void foobar();

    /*========================================================================*/ // EXPECT_WARNING
    void update();

    /*========================================================================*/void inlineCode();
    // EXPECT_WARNING_ABOVE

    // A short explanation that ends with a full stop.
    void flush();

    // int legacyValue = 0;
    void render();

    // if (enabled) {
    void draw();

    // NOLINTNEXTLINE(readability-identifier-naming)
    void paint();

    // See the docs (section 3).
    void resized();
};

int trailing = 0; //==========

int trailingBlock = 0; /*----------*/
