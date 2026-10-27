// A minimal stand-in for the parts of the JUCE API that the checks care about.
// Signatures mirror JUCE's own so the tests exercise the same AST shapes as real
// projects, without the cost of parsing the real JUCE modules for every test.

#pragma once

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define JUCE_MAC 1
#define JUCE_DEBUG 1
#define JUCE_CLANG 1
#define JUCE_64BIT 1
#define JUCE_ARM 1
#define JUCE_LITTLE_ENDIAN 1

#define JUCE_API
#define JUCE_JOIN_MACRO_HELPER(a, b) a##b
#define JUCE_JOIN_MACRO(a, b) JUCE_JOIN_MACRO_HELPER (a, b)
#define JUCE_BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)

namespace juce
{
using int8 = signed char;
using uint8 = unsigned char;
using int16 = signed short;
using uint16 = unsigned short;
using int32 = signed int;
using uint32 = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

void logAssertion (const char* file, int line) noexcept;

template <typename... Types>
void ignoreUnused (Types&&...) noexcept
{
}

template <typename OwnerClass>
class LeakedObjectDetector
{
public:
    LeakedObjectDetector() noexcept {}
    LeakedObjectDetector (const LeakedObjectDetector&) noexcept {}
    ~LeakedObjectDetector() {}
};

template <typename Type>
void deleteAndZero (Type& pointer)
{
    delete pointer;
    pointer = nullptr;
}

template <typename Type>
constexpr Type jmax (Type a, Type b);
template <typename Type>
constexpr Type jmax (Type a, Type b, Type c);
template <typename Type>
constexpr Type jmax (Type a, Type b, Type c, Type d);

template <typename Type>
constexpr Type jmin (Type a, Type b);
template <typename Type>
constexpr Type jmin (Type a, Type b, Type c);
template <typename Type>
constexpr Type jmin (Type a, Type b, Type c, Type d);

template <typename Type>
Type jlimit (Type lowerLimit, Type upperLimit, Type valueToConstrain) noexcept;

template <typename Type>
constexpr bool exactlyEqual (Type a, Type b);

template <typename Type>
constexpr bool approximatelyEqual (Type a, Type b);

template <typename Type>
Type juce_hypot (Type a, Type b) noexcept;

template <>
float juce_hypot (float a, float b) noexcept;

template <typename NumericType>
bool juce_isfinite (NumericType value) noexcept;

class StringRef;

class String
{
public:
    String() noexcept;
    String (const String&) noexcept;
    String (String&&) noexcept;
    String (const char*);
    String (const char*, size_t maxChars);
   #if __cpp_char8_t
    String (const char8_t*);
    String (const char8_t*, size_t maxChars);
   #endif
    String (const std::string&);
    explicit String (int);
    explicit String (float);
    ~String();

    String& operator= (const String&) noexcept;
    String& operator+= (const String&);
    String& operator<< (const String&);
    String& operator<< (int);

    static String formatted (const String& format, ...);
    static String fromUTF8 (const char* utf8buffer, int bufferSizeBytes = -1);
   #if __cpp_char8_t
    static String fromUTF8 (const char8_t* utf8buffer, int bufferSizeBytes = -1);
   #endif

    String trim() const;
    String toLowerCase() const;
    String substring (int startIndex) const;
    std::string toStdString() const;
    bool equalsIgnoreCase (StringRef other) const noexcept;
    bool startsWith (StringRef text) const noexcept;
    String dropLastCharacters (int numberToDrop) const;
    String toUpperCase() const;
    String replace (StringRef stringToReplace, StringRef stringToInsertInstead, bool ignoreCase = false) const;

    int length() const noexcept;
    bool isEmpty() const noexcept;
};

String operator+ (const String&, const String&);
bool operator== (const String&, const char*) noexcept;
bool operator== (const String&, const String&) noexcept;
String operator+ (const char*, const String&);
String operator+ (const String&, const char*);

class StringRef
{
public:
    StringRef() noexcept;
    StringRef (const char*) noexcept;
    StringRef (const String&) noexcept;
};

class Identifier
{
public:
    Identifier (const char*);
    Identifier (const String&);
    operator StringRef() const noexcept;
};

class CharPointer_UTF8
{
public:
    explicit CharPointer_UTF8 (const char*) noexcept;
};

class Colour
{
public:
    constexpr Colour() noexcept = default;
    constexpr explicit Colour (uint32 argb) noexcept : argb (argb) {}

private:
    uint32 argb = 0;
};

template <typename ValueType>
class Point
{
public:
    constexpr Point() = default;
    constexpr Point (ValueType x, ValueType y) noexcept : x (x), y (y) {}

    ValueType x {};
    ValueType y {};
};

template <typename ValueType>
class Rectangle
{
public:
    Rectangle() = default;
    Rectangle (ValueType x, ValueType y, ValueType w, ValueType h) noexcept;
    void setWidth (ValueType newWidth) noexcept;
    void setHeight (ValueType newHeight) noexcept;
    void reduce (ValueType deltaX, ValueType deltaY) noexcept;
    Rectangle reduced (ValueType delta) const noexcept;
    Rectangle withSizeKeepingCentre (ValueType newWidth, ValueType newHeight) const noexcept;
    Rectangle<float> toFloat() const noexcept;
};

template <typename ValueType>
class Range
{
public:
    constexpr Range() = default;
    constexpr Range (ValueType start, ValueType end) noexcept : start (start), end (end) {}

private:
    ValueType start {};
    ValueType end {};
};

class Time
{
public:
    static Time getCurrentTime() noexcept;
    static int64 currentTimeMillis() noexcept;
    static uint32 getMillisecondCounter() noexcept;
    static double getMillisecondCounterHiRes() noexcept;
    static int64 getHighResolutionTicks() noexcept;
};

class RelativeTime
{
public:
    explicit RelativeTime (double seconds = 0.0) noexcept;
};

class ModifierKeys
{
};

class Justification
{
public:
    Justification (int flags) noexcept;
};

class PixelARGB
{
};

class PixelRGB
{
};

template <typename ElementType>
class Array
{
public:
    void add (const ElementType&);
    void insert (int index, const ElementType&);
    void resize (int size);
    void ensureStorageAllocated (int size);
    void clear();
    int size() const noexcept;
    ElementType& getReference (int index) noexcept;
    ElementType operator[] (int index) const;
};

template <typename ObjectClass>
class OwnedArray
{
public:
    ObjectClass* add (ObjectClass*);
    ObjectClass* operator[] (int index) const noexcept;
    int size() const noexcept;
};

inline constexpr size_t dynamicExtent = std::numeric_limits<size_t>::max();

template <typename Value, size_t Extent = dynamicExtent>
class Span
{
public:
    constexpr Span() = default;
    constexpr Span (Value* ptr, size_t size);
    constexpr size_t size() const;
};

template <typename Value>
class Optional
{
public:
    Optional() = default;
    Optional (Value value);
    bool hasValue() const noexcept;
};

template <typename Type>
struct Atomic
{
    Atomic() noexcept;
    Atomic (Type initialValue) noexcept;
    Type get() const noexcept;
    void set (Type newValue) noexcept;
};

template <typename FloatType>
struct MathConstants
{
    static constexpr FloatType pi = static_cast<FloatType> (3.141592653589793238L);
    static constexpr FloatType twoPi = static_cast<FloatType> (2 * 3.141592653589793238L);
    static constexpr FloatType halfPi = static_cast<FloatType> (3.141592653589793238L / 2);
    static constexpr FloatType euler = static_cast<FloatType> (2.71828182845904523536L);
    static constexpr FloatType sqrt2 = static_cast<FloatType> (1.4142135623730950488L);
};

template <typename KeyType, typename ValueType>
class HashMap
{
public:
    void set (KeyType newKey, ValueType newValue);
    int size() const noexcept;
};

template <class ElementType>
class SortedSet
{
public:
    bool add (const ElementType& newElement) noexcept;
    int size() const noexcept;
};

template <class ListenerClass>
class ListenerList
{
public:
    void add (ListenerClass* listenerToAdd);
    void remove (ListenerClass* listenerToRemove);

    template <typename Callback>
    void call (Callback&& callback);
};

template <typename ElementType>
class HeapBlock
{
public:
    HeapBlock() = default;
    explicit HeapBlock (size_t numElements);
    void malloc (size_t numElements);
    void allocate (size_t numElements, bool initialiseToZero);
    ElementType* get() const noexcept;
};

class MemoryBlock;
class ReferenceCountedObject;
class DynamicObject;

class var
{
public:
    struct NativeFunctionArgs;
    using NativeFunction = std::function<var (const NativeFunctionArgs&)>;

    var() noexcept;
    var (int);
    var (const char*);
    var (const String&);

    bool isArray() const noexcept;
    bool isObject() const noexcept;
    bool isBinaryData() const noexcept;
    Array<var>* getArray() const noexcept;
    MemoryBlock* getBinaryData() const noexcept;
    ReferenceCountedObject* getObject() const noexcept;
    DynamicObject* getDynamicObject() const noexcept;
    NativeFunction getNativeFunction() const;
};

class ReferenceCountedObject
{
public:
    virtual ~ReferenceCountedObject();
};

class DynamicObject : public ReferenceCountedObject
{
public:
    const var& getProperty (const Identifier& propertyName) const;
    void setProperty (const Identifier& propertyName, const var& newValue);
};

class XmlElement
{
public:
    String getTagName() const;
    bool hasTagName (StringRef possibleTagName) const noexcept;
    XmlElement* getChildByName (StringRef tagNameToLookFor) const noexcept;
    XmlElement* getFirstChildElement() const noexcept;
};

std::unique_ptr<XmlElement> parseXML (const String& textToParse);

class UndoManager
{
public:
    bool undo();
    void beginNewTransaction();
};

class ValueTree
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void valueTreePropertyChanged (ValueTree&, const Identifier&) {}
        virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) {}
        virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {}
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) {}
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) {}
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {}
    };

    ValueTree() noexcept;
    explicit ValueTree (const Identifier& type);
    ValueTree (const ValueTree&) noexcept;
    ValueTree& operator= (const ValueTree&);

    bool operator== (const ValueTree&) const noexcept;
    bool operator!= (const ValueTree&) const noexcept;
    bool isEquivalentTo (const ValueTree&) const;
    bool isValid() const noexcept;
    ValueTree createCopy() const;
    Identifier getType() const noexcept;
    bool hasType (const Identifier& typeName) const noexcept;

    const var& getProperty (const Identifier&) const noexcept;
    ValueTree& setProperty (const Identifier&, const var&, UndoManager* undoManager);
    void removeProperty (const Identifier& name, UndoManager* undoManager);
    ValueTree getChild (int index) const;
    ValueTree getChildWithName (const Identifier& type) const;
    ValueTree getOrCreateChildWithName (const Identifier& type, UndoManager* undoManager);
    ValueTree getChildWithProperty (const Identifier& propertyName, const var& propertyValue) const;
    ValueTree getParent() const noexcept;
    ValueTree getSibling (int delta) const noexcept;
    ValueTree getRoot() const noexcept;
    void appendChild (const ValueTree& child, UndoManager* undoManager);
    void addChild (const ValueTree& child, int index, UndoManager* undoManager);
    void removeChild (const ValueTree& child, UndoManager* undoManager);
    void removeAllChildren (UndoManager* undoManager);
    void removeAllProperties (UndoManager* undoManager);
    void moveChild (int currentIndex, int newIndex, UndoManager* undoManager);
    std::unique_ptr<XmlElement> createXml() const;
    static ValueTree fromXml (const XmlElement& xml);
    static ValueTree readFromData (const void* data, size_t numBytes);
    void addListener (Listener* listener);
    void removeListener (Listener* listener);
};

template <typename Type>
class CachedValue : private ValueTree::Listener
{
public:
    CachedValue();
    CachedValue (ValueTree& tree, const Identifier& propertyID, UndoManager* undoManager);
    void referTo (ValueTree& tree, const Identifier& property, UndoManager* undoManager);
    Type get() const noexcept;
    operator Type() const noexcept;
    CachedValue& operator= (const Type& newValue);
};

class Value
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void valueChanged (Value&) = 0;
    };

    Value();
    Value (const Value&);
    void addListener (Listener* listener);
    void removeListener (Listener* listener);
};

class MemoryBlock
{
public:
    void setSize (size_t newSize, bool initialiseNewSpaceToZero = false);
};

template <typename ObjectType>
class WeakReference
{
public:
    WeakReference() noexcept = default;
    WeakReference (ObjectType*);
    ObjectType* get() const noexcept;
    operator ObjectType*() const noexcept;
    ObjectType* operator->() const noexcept;
};

template <typename ObjectType>
class ReferenceCountedObjectPtr
{
public:
    ReferenceCountedObjectPtr() noexcept = default;
    ReferenceCountedObjectPtr (ObjectType*) noexcept;
    operator ObjectType*() const noexcept;
    ObjectType* operator->() const noexcept;
};

//==============================================================================
class CriticalSection
{
public:
    void enter() const noexcept;
    bool tryEnter() const noexcept;
    void exit() const noexcept;
};

class SpinLock
{
public:
    void enter() const noexcept;
    bool tryEnter() const noexcept;
    void exit() const noexcept;
};

template <class LockType>
class GenericScopedLock
{
public:
    explicit GenericScopedLock (const LockType&) noexcept;
    ~GenericScopedLock() noexcept;
};

template <class LockType>
class GenericScopedTryLock
{
public:
    explicit GenericScopedTryLock (const LockType&, bool acquireLockOnInitialisation = true) noexcept;
    ~GenericScopedTryLock() noexcept;
    bool isLocked() const noexcept;
};

using ScopedLock = GenericScopedLock<CriticalSection>;
using ScopedTryLock = GenericScopedTryLock<CriticalSection>;

class WaitableEvent
{
public:
    bool wait (double timeOutMilliseconds = -1.0) const;
    void signal() const;
};

class Logger
{
public:
    virtual ~Logger();

    static void setCurrentLogger (Logger* newLogger) noexcept;
    static void writeToLog (const String& message);
    static void outputDebugString (const String& text);

protected:
    virtual void logMessage (const String& message) = 0;
};

class File
{
public:
    enum SpecialLocationType
    {
        userHomeDirectory,
        userApplicationDataDirectory,
        tempDirectory,
        currentApplicationFile
    };

    File() = default;
    File (const String& absolutePath);
    bool exists() const;
    File getChildFile (StringRef relativeOrAbsolutePath) const;
    static File getCurrentWorkingDirectory();
    static File getSpecialLocation (const SpecialLocationType type);
    bool replaceWithText (const String& text) const;
};

class StringArray;

class URL
{
public:
    explicit URL (const String& url);
    String getOrigin() const;
};

template <typename OptionsType>
class OptionsBuilder
{
public:
    virtual ~OptionsBuilder() = default;
    virtual OptionsType buildOptions (const OptionsType& initialOptions) = 0;
};

class MessageManager
{
public:
    static MessageManager* getInstance();
    static MessageManager* getInstanceWithoutCreating() noexcept;
    static bool callAsync (std::function<void()> functionToCall);
    void* callFunctionOnMessageThread (void* (*func) (void*), void* userData);
    bool isThisTheMessageThread() const noexcept;
};

class Thread;

class MessageManagerLock
{
public:
    explicit MessageManagerLock (Thread* threadToCheckForExitSignal = nullptr);
    ~MessageManagerLock() noexcept;
    bool lockWasGained() const noexcept;
};

class ChangeBroadcaster;

class ChangeListener
{
public:
    virtual ~ChangeListener() = default;
    virtual void changeListenerCallback (ChangeBroadcaster* source) = 0;
};

class ChangeBroadcaster
{
public:
    virtual ~ChangeBroadcaster();
    void addChangeListener (ChangeListener* listener);
    void removeChangeListener (ChangeListener* listener);
    void sendChangeMessage();
    void sendSynchronousChangeMessage();
};

class AsyncUpdater
{
public:
    virtual ~AsyncUpdater();
    virtual void handleAsyncUpdate() = 0;
    void triggerAsyncUpdate();
};

class Timer
{
public:
    virtual ~Timer();
    virtual void timerCallback() = 0;
    void startTimer (int intervalInMilliseconds) noexcept;
    void startTimerHz (int timerFrequencyHz) noexcept;
    void stopTimer() noexcept;
    static void callAfterDelay (int milliseconds, std::function<void()> functionToCall);
};

class TimedCallback final : private Timer
{
public:
    explicit TimedCallback (std::function<void()> callbackIn);
    using Timer::startTimer;
    using Timer::startTimerHz;
    using Timer::stopTimer;

private:
    void timerCallback() override;
};

class HighResolutionTimer
{
public:
    virtual ~HighResolutionTimer();
    virtual void hiResTimerCallback() = 0;
    void startTimer (int intervalInMilliseconds);
    void stopTimer();
};

class Thread
{
public:
    explicit Thread (const String& threadName, size_t threadStackSize = 0);
    virtual ~Thread();
    virtual void run() = 0;
    bool startThread();
    bool stopThread (int timeOutMilliseconds);
    void signalThreadShouldExit();
    bool threadShouldExit() const;
    static bool currentThreadShouldExit();
    static Thread* getCurrentThread();
    bool wait (double timeOutMilliseconds) const;
    static void sleep (int milliseconds);
    static bool launch (std::function<void()> functionToRun);
};

//==============================================================================
class Image
{
public:
    enum PixelFormat
    {
        UnknownFormat,
        RGB,
        ARGB,
        SingleChannel
    };

    Image() noexcept;
    Image (PixelFormat format, int imageWidth, int imageHeight, bool clearImage);
    Image (const Image&) noexcept;
};

class ImageFileFormat
{
public:
    static Image loadFrom (const void* rawData, size_t numBytesOfData);
};

class ImageCache
{
public:
    static Image getFromMemory (const void* imageData, int dataSize);
};

class Drawable
{
public:
    virtual ~Drawable();
    static std::unique_ptr<Drawable> createFromImageData (const void* data, size_t numBytes);
};

class AffineTransform
{
public:
    AffineTransform() noexcept;
    static AffineTransform translation (float deltaX, float deltaY) noexcept;
};

class Path
{
public:
    void startNewSubPath (float startX, float startY);
    void lineTo (float endX, float endY);
    void closeSubPath();
    void addRoundedRectangle (float x, float y, float width, float height, float cornerSize);
    template <typename ValueType>
    void addRectangle (Rectangle<ValueType> rectangle);
    template <typename ValueType>
    void addRoundedRectangle (Rectangle<ValueType> rectangle, float cornerSize);
    void addEllipse (Rectangle<float> area);
    void applyTransform (const AffineTransform& transform) noexcept;
};

class PathStrokeType
{
public:
    PathStrokeType (float strokeThickness) noexcept;
};

class ColourGradient
{
public:
    ColourGradient() noexcept;
    ColourGradient (Colour colour1, float x1, float y1, Colour colour2, float x2, float y2, bool isRadial);
    static ColourGradient vertical (Colour colour1, float y1, Colour colour2, float y2);
};

class Graphics
{
public:
    void fillAll (Colour) const;
    void fillPath (const Path& path) const;
    void strokePath (const Path& path, const PathStrokeType& strokeType, const AffineTransform& transform = {}) const;
    void drawRoundedRectangle (Rectangle<float> rectangle, float cornerSize, float lineThickness) const;
    void drawEllipse (Rectangle<float> area, float lineThickness) const;
    void drawRect (Rectangle<float> rectangle, float lineThickness = 1.0f) const;
    void setGradientFill (const ColourGradient& gradient);
    void fillAll() const;
    void fillRect (Rectangle<int> area) const;
    void setColour (Colour newColour);
    void drawImageAt (const Image& imageToDraw, int topLeftX, int topLeftY, bool fillAlphaChannelWithCurrentBrush = false) const;
    void saveState();
    void restoreState();

    class ScopedSaveState
    {
    public:
        explicit ScopedSaveState (Graphics&);
        ~ScopedSaveState();
    };
};

class MouseEvent
{
};

class Button;
class TextButton;

class LookAndFeel
{
public:
    virtual ~LookAndFeel();
    static void setDefaultLookAndFeel (LookAndFeel* newDefaultLookAndFeel) noexcept;

    virtual void drawButtonBackground (Graphics&, Button&, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown);
    virtual int getTextButtonWidthToFitText (TextButton&, int buttonHeight);
};

class LookAndFeel_V4 : public LookAndFeel
{
};

class ComponentListener;
class Component;

class AccessibilityHandler
{
public:
    virtual ~AccessibilityHandler();
};

class KeyPress
{
};

class Component
{
public:
    Component() noexcept;
    virtual ~Component();

    Component (const Component&) = delete;
    Component& operator= (const Component&) = delete;

    virtual void paint (Graphics&);
    virtual void resized();
    virtual void paintOverChildren (Graphics&);
    virtual void mouseDown (const MouseEvent&);
    virtual void mouseUp (const MouseEvent&);
    virtual void mouseDrag (const MouseEvent&);
    virtual void mouseEnter (const MouseEvent&);
    virtual void mouseDoubleClick (const MouseEvent&);
    virtual bool keyPressed (const KeyPress&);
    virtual std::unique_ptr<AccessibilityHandler> createAccessibilityHandler();

    void addAndMakeVisible (Component* child, int zOrder = -1);
    void addAndMakeVisible (Component& child, int zOrder = -1);
    void addChildComponent (Component* child, int zOrder = -1);
    void addChildComponent (Component& child, int zOrder = -1);
    void deleteAllChildren();
    void removeChildComponent (Component* child);
    void removeAllChildren();

    void setSize (int width, int height);
    void setBounds (int x, int y, int width, int height);
    void repaint();
    void setLookAndFeel (LookAndFeel* newLookAndFeel);
    void setBounds (Rectangle<int> newBounds);
    void setTopLeftPosition (int x, int y);
    void setVisible (bool shouldBeVisible);
    void setOpaque (bool shouldBeOpaque);
    void setBufferedToImage (bool shouldBeBuffered);
    void toFront (bool shouldAlsoGainKeyboardFocus);
    void toBack();
    void toBehind (Component* other);
    void addToDesktop (int windowStyleFlags, void* nativeWindowToAttachTo = nullptr);
    Rectangle<int> getLocalBounds() const noexcept;
    int getWidth() const noexcept;
    int getHeight() const noexcept;

    void setName (const String& newName);
    void setTitle (const String& newTitle);
    void setDescription (const String& newDescription);
    void setHelpText (const String& newHelpText);
    void setAccessible (bool shouldBeAccessible);
    void setWantsKeyboardFocus (bool wantsFocus) noexcept;

    Component* getParentComponent() const noexcept;
    Component* getChildComponent (int index) const noexcept;
    void addComponentListener (ComponentListener* listener);
    void removeComponentListener (ComponentListener* listener);
    void addMouseListener (Component* newListener, bool wantsEventsForAllNestedChildComponents);
    void removeMouseListener (Component* listenerToRemove);

    template <class TargetClass>
    TargetClass* findParentComponentOfClass() const;

    template <class ComponentType>
    class SafePointer
    {
    public:
        SafePointer() noexcept = default;
        SafePointer (ComponentType* component);
        ComponentType* getComponent() const noexcept;
        operator ComponentType*() const noexcept;
        ComponentType* operator->() const noexcept;
    };

    class BailOutChecker
    {
    public:
        BailOutChecker (Component* component);
        bool shouldBailOut() const noexcept;
    };
};

class VBlankAttachment
{
public:
    VBlankAttachment (Component* c, std::function<void()> callback);
};

class ComponentListener
{
public:
    virtual ~ComponentListener() = default;
    virtual void componentMovedOrResized (Component&, bool wasMoved, bool wasResized);
};

class Button : public Component
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void buttonClicked (Button*) = 0;
    };

    void addListener (Listener* listener);
    void removeListener (Listener* listener);
    void setButtonText (const String& newText);
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

    std::function<void()> onClick;
};

class TextButton : public Button
{
public:
    TextButton();
    explicit TextButton (const String& buttonName);
};

class Slider : public Component
{
public:
    Slider();
    explicit Slider (const String& componentName);

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void sliderValueChanged (Slider*) = 0;
    };

    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;
        virtual void drawRotarySlider (Graphics&, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider&) = 0;
    };

    void addListener (Listener* listener);
    void removeListener (Listener* listener);
    double getValue() const;
    void paint (Graphics&) override;
    void resized() override;
    void mouseDown (const MouseEvent&) override;
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

    std::function<void()> onValueChange;
};

class Label : public Component
{
public:
    Label();
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;
};

class ToggleButton : public Button
{
public:
    ToggleButton();
    explicit ToggleButton (const String& buttonText);
};

class ImageButton : public Button
{
public:
    explicit ImageButton (const String& name = String());
};

class ComboBox : public Component
{
public:
    explicit ComboBox (const String& componentName = {});
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

    std::function<void()> onChange;
};

class TextEditor : public Component
{
public:
    explicit TextEditor (const String& componentName = String());
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;
};

class Viewport : public Component
{
public:
    void setViewedComponent (Component* newViewedComponent, bool deleteComponentWhenNoLongerNeeded = true);
};

class FlexItem
{
public:
    FlexItem() noexcept;
    FlexItem (Component& componentToControl) noexcept;
};

class FlexBox
{
public:
    void performLayout (Rectangle<int> targetArea);
    Array<FlexItem> items;
};

class File;

class FileChooser
{
public:
    explicit FileChooser (const String& dialogBoxTitle);
    ~FileChooser();
    void launchAsync (int flags, std::function<void (const FileChooser&)> callback, void* previewComponent = nullptr);
};

class OpenGLContext
{
public:
    OpenGLContext();
    ~OpenGLContext();
    void attachTo (Component& component);
    void detach();
};

class TopLevelWindow : public Component
{
};

class ResizableWindow : public TopLevelWindow
{
public:
    void setContentOwned (Component* newContentComponent, bool resizeToFitWhenContentChangesSize);
    void setContentNonOwned (Component* newContentComponent, bool resizeToFitWhenContentChangesSize);
};

class DocumentWindow : public ResizableWindow
{
};

class TooltipWindow : public Component
{
public:
    explicit TooltipWindow (Component* parentComponent = nullptr, int millisecondsBeforeTipAppears = 700);
};

class NativeMessageBox
{
public:
    static void showAsync (const String& message, std::function<void (int)> callback);
};

class WebBrowserComponent;

class WebViewLifetimeListener
{
public:
    virtual ~WebViewLifetimeListener() = default;
    virtual void webViewConstructed (WebBrowserComponent*) = 0;
    virtual void webViewDestructed (WebBrowserComponent*) = 0;
};

class WebBrowserComponent : public Component
{
public:
    using NativeEventListener = std::function<void (var)>;
    using NativeFunctionCompletion = std::function<void (var)>;
    using NativeFunction = std::function<void (const Array<var>&, NativeFunctionCompletion)>;

    struct Resource
    {
        std::vector<std::byte> data;
        String mimeType;
    };

    using ResourceProvider = std::function<std::optional<Resource> (const String&)>;

    class Options
    {
    public:
        enum class Backend
        {
            defaultBackend,
            ie,
            webview2
        };

        class WinWebView2
        {
        public:
            [[nodiscard]] WinWebView2 withDLLLocation (const File& location) const;
            [[nodiscard]] WinWebView2 withUserDataFolder (const File& folder) const;
            [[nodiscard]] WinWebView2 withStatusBarDisabled() const;
            [[nodiscard]] WinWebView2 withBuiltInErrorPageDisabled() const;

        private:
            bool disableStatusBar = false;
        };

        [[nodiscard]] Options withBackend (Backend backend) const;
        [[nodiscard]] Options withKeepPageLoadedWhenBrowserIsHidden() const;
        [[nodiscard]] Options withUserAgent (String ua) const;
        [[nodiscard]] Options withWinWebView2Options (const WinWebView2& winWebView2Options) const;
        [[nodiscard]] Options withNativeIntegrationEnabled (bool enabled = true) const;
        [[nodiscard]] Options withNativeFunction (const Identifier& name, NativeFunction callback) const;
        [[nodiscard]] Options withEventListener (const Identifier& eventId, NativeEventListener listener) const;
        [[nodiscard]] Options withUserScript (StringRef script) const;
        [[nodiscard]] Options withInitialisationData (StringRef name, const var& value) const;
        [[nodiscard]] Options withResourceProvider (ResourceProvider provider, std::optional<String> allowedOriginIn = std::nullopt) const;
        [[nodiscard]] Options withWebViewLifetimeListener (WebViewLifetimeListener* listener);

        template <typename OptionsType>
        OptionsType withOptionsFrom (OptionsBuilder<OptionsType>& builder) const
        {
            return builder.buildOptions (static_cast<const OptionsType&> (*this));
        }

    private:
        bool enableNativeIntegration = false;
    };

    class EvaluationResult
    {
    public:
        struct Error
        {
            String message;
        };

        EvaluationResult (const var& result);
        EvaluationResult (const Error& error);

        const var* getResult() const;
        const Error* getError() const;
    };

    using EvaluationCallback = std::function<void (EvaluationResult)>;

    WebBrowserComponent();
    explicit WebBrowserComponent (const Options& options);
    ~WebBrowserComponent() override;

    void goToURL (const String& url, const StringArray* headers = nullptr, const MemoryBlock* postData = nullptr);
    static const String& getResourceProviderRoot();
    void evaluateJavascript (const String& script, EvaluationCallback callback = nullptr);
    void emitEventIfBrowserIsVisible (const Identifier& eventId, const var& object);

    virtual bool pageAboutToLoad (const String& newURL);
    virtual void pageFinishedLoading (const String& url);
    virtual bool pageLoadHadNetworkError (const String& errorInfo);
};

class WebSliderRelay : public OptionsBuilder<WebBrowserComponent::Options>,
                       private WebViewLifetimeListener
{
public:
    WebSliderRelay (StringRef nameIn);
    void setValue (float newValue);
    float getValue() const;
    WebBrowserComponent::Options buildOptions (const WebBrowserComponent::Options& initialOptions) override;

private:
    void webViewConstructed (WebBrowserComponent*) override;
    void webViewDestructed (WebBrowserComponent*) override;
};

class WebToggleButtonRelay : public OptionsBuilder<WebBrowserComponent::Options>,
                             private WebViewLifetimeListener
{
public:
    WebToggleButtonRelay (StringRef nameIn);
    void setToggleState (bool newState);
    WebBrowserComponent::Options buildOptions (const WebBrowserComponent::Options& initialOptions) override;

private:
    void webViewConstructed (WebBrowserComponent*) override;
    void webViewDestructed (WebBrowserComponent*) override;
};

class WebComboBoxRelay : public OptionsBuilder<WebBrowserComponent::Options>,
                         private WebViewLifetimeListener
{
public:
    WebComboBoxRelay (StringRef nameIn);
    void setValue (float newValue);
    WebBrowserComponent::Options buildOptions (const WebBrowserComponent::Options& initialOptions) override;

private:
    void webViewConstructed (WebBrowserComponent*) override;
    void webViewDestructed (WebBrowserComponent*) override;
};

class WebControlParameterIndexReceiver : public OptionsBuilder<WebBrowserComponent::Options>
{
public:
    int getControlParameterIndex() const;
    WebBrowserComponent::Options buildOptions (const WebBrowserComponent::Options& initialOptions) override;
};

//==============================================================================
template <typename Type>
class AudioBuffer
{
public:
    AudioBuffer() noexcept;
    AudioBuffer (int numChannels, int numSamples);
    AudioBuffer (const AudioBuffer& other);
    AudioBuffer& operator= (const AudioBuffer& other);

    int getNumChannels() const noexcept;
    int getNumSamples() const noexcept;
    const Type* getReadPointer (int channel) const noexcept;
    const Type* getReadPointer (int channel, int sampleIndex) const noexcept;
    Type* getWritePointer (int channel) noexcept;
    Type* getWritePointer (int channel, int sampleIndex) noexcept;
    const Type* const* getArrayOfReadPointers() const noexcept;
    Type* const* getArrayOfWritePointers() noexcept;
    Type getSample (int channel, int sampleIndex) const noexcept;
    void setSample (int destChannel, int destSample, Type newValue) noexcept;
    void addSample (int destChannel, int destSample, Type valueToAdd) noexcept;
    void clear() noexcept;
    void applyGain (Type gain) noexcept;
    void setSize (int newNumChannels,
                  int newNumSamples,
                  bool keepExistingContent = false,
                  bool clearExtraSpace = false,
                  bool avoidReallocating = false);
    void makeCopyOf (const AudioBuffer& other, bool avoidReallocating = false);
};

namespace ValueSmoothingTypes
{
struct Linear
{
};
} // namespace ValueSmoothingTypes

template <typename FloatType, typename SmoothingType = ValueSmoothingTypes::Linear>
class SmoothedValue
{
public:
    SmoothedValue() noexcept;
    void reset (double sampleRate, double rampLengthInSeconds) noexcept;
    void reset (int numSteps) noexcept;
    void setTargetValue (FloatType newValue) noexcept;
    FloatType getNextValue() noexcept;
};

class ADSR
{
public:
    void setSampleRate (double sampleRate) noexcept;
    float getNextSample() noexcept;
};

class Reverb
{
public:
    void setSampleRate (double sampleRate);
    void processStereo (float* left, float* right, int numSamples) noexcept;
};

namespace dsp
{
struct ProcessSpec
{
    double sampleRate;
    uint32 maximumBlockSize;
    uint32 numChannels;
};

template <typename SampleType>
class Gain
{
public:
    void prepare (const ProcessSpec& spec) noexcept;
    void reset() noexcept;
    void setGainDecibels (SampleType newGainDecibels) noexcept;
    SampleType processSample (SampleType sample) noexcept;
};

namespace IIR
{
template <typename NumericType>
class Coefficients
{
public:
    using Ptr = ReferenceCountedObjectPtr<Coefficients>;
    static Ptr makeLowPass (double sampleRate, NumericType frequency);
};

template <typename SampleType>
class Filter
{
public:
    void prepare (const ProcessSpec&) noexcept;
    void reset() noexcept;
    SampleType processSample (SampleType sample) noexcept;
    typename Coefficients<SampleType>::Ptr coefficients;
};
} // namespace IIR

template <typename SampleType>
class Oversampling
{
public:
    void initProcessing (size_t maximumNumberOfSamplesBeforeOversampling);
};
} // namespace dsp

class MidiBuffer
{
public:
    void clear() noexcept;
};

class ScopedNoDenormals
{
public:
    ScopedNoDenormals() noexcept;
    ~ScopedNoDenormals() noexcept;
};

class FloatVectorOperations
{
public:
    static void disableDenormalisedNumberSupport (bool shouldDisable = true) noexcept;
};

class AudioPlayHead
{
public:
    struct PositionInfo
    {
        double getBpm() const;
    };

    virtual ~AudioPlayHead() = default;
    virtual PositionInfo getPosition() const = 0;
};

class ParameterID
{
public:
    ParameterID() = default;

    template <typename StringLike, typename = std::enable_if_t<! std::is_base_of_v<ParameterID, std::decay_t<StringLike>>>>
    ParameterID (StringLike&& identifier, int versionHint = 0)
        : paramID (std::forward<StringLike> (identifier)), version (versionHint)
    {
    }

    String getParamID() const;
    int getVersionHint() const;

private:
    String paramID;
    int version = 0;
};

template <typename ValueType>
class NormalisableRange
{
public:
    NormalisableRange() = default;
    NormalisableRange (ValueType rangeStart, ValueType rangeEnd);
};

class AudioProcessorParameter
{
public:
    virtual ~AudioProcessorParameter();
    virtual float getValue() const = 0;

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void parameterValueChanged (int parameterIndex, float newValue) = 0;
        virtual void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) = 0;
    };

    void addListener (Listener* newListener);
    void removeListener (Listener* listenerToRemove);
};

class AudioProcessorParameterWithID : public AudioProcessorParameter
{
public:
    AudioProcessorParameterWithID (const ParameterID& parameterID, const String& parameterName);
};

class RangedAudioParameter : public AudioProcessorParameterWithID
{
public:
    using AudioProcessorParameterWithID::AudioProcessorParameterWithID;
    float convertFrom0to1 (float proportion) const noexcept;
};

class AudioParameterFloat : public RangedAudioParameter
{
public:
    AudioParameterFloat (const ParameterID& parameterID,
                         const String& parameterName,
                         NormalisableRange<float> normalisableRange,
                         float defaultValue);
    AudioParameterFloat (const ParameterID& parameterID,
                         const String& parameterName,
                         float minValue,
                         float maxValue,
                         float defaultValue);
    float get() const noexcept;
    operator float() const noexcept;

protected:
    virtual void valueChanged (float newValue);

private:
    float getValue() const override;
};

class AudioParameterBool : public RangedAudioParameter
{
public:
    AudioParameterBool (const ParameterID& parameterID, const String& parameterName, bool defaultValue);
    bool get() const noexcept;

private:
    float getValue() const override;
};

class AudioProcessorEditor;
class AudioProcessor;

class AudioProcessorListener
{
public:
    virtual ~AudioProcessorListener() = default;
    virtual void audioProcessorParameterChanged (AudioProcessor* processor, int parameterIndex, float newValue) = 0;
};

class AudioProcessor
{
public:
    AudioProcessor();
    virtual ~AudioProcessor();

    virtual void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) = 0;
    virtual void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual void getStateInformation (MemoryBlock& destData) = 0;
    virtual void setStateInformation (const void* data, int sizeInBytes) = 0;
    virtual int getNumPrograms() = 0;
    virtual int getCurrentProgram() = 0;
    virtual void setCurrentProgram (int index) = 0;
    virtual void numChannelsChanged();

    class Bus
    {
    public:
        int getNumberOfChannels() const noexcept;
    };

    Bus* getBus (bool isInput, int busIndex) noexcept;
    double getSampleRate() const noexcept;
    int getBlockSize() const noexcept;
    void setLatencySamples (int newLatency);
    void setPlayConfigDetails (int numIns, int numOuts, double sampleRate, int blockSize);
    void setRateAndBufferSizeDetails (double sampleRate, int blockSize) noexcept;

    static void copyXmlToBinary (const XmlElement& xml, MemoryBlock& destData);
    static std::unique_ptr<XmlElement> getXmlFromBinary (const void* data, int sizeInBytes);

    AudioPlayHead* getPlayHead() const noexcept;
    AudioProcessorEditor* getActiveEditor() const noexcept;
    void addParameter (AudioProcessorParameter* parameter);
};

class AudioProcessorEditor : public Component
{
public:
    explicit AudioProcessorEditor (AudioProcessor&) noexcept;
    ~AudioProcessorEditor() override;

    AudioProcessor& processor;
};

class AudioProcessorValueTreeState
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void parameterChanged (const String& parameterID, float newValue) = 0;
    };

    class ParameterLayout
    {
    public:
        ParameterLayout() = default;

        template <typename... Items>
        ParameterLayout (std::unique_ptr<Items>... items);

        template <typename... Items>
        void add (std::unique_ptr<Items>... items);
    };

    AudioProcessorValueTreeState (AudioProcessor& processor, void* undoManager, const Identifier& valueTreeType, ParameterLayout parameterLayout);

    RangedAudioParameter* getParameter (StringRef parameterID) const noexcept;
    std::atomic<float>* getRawParameterValue (StringRef parameterID) const noexcept;
    void addParameterListener (StringRef parameterID, Listener* listener);
    void removeParameterListener (StringRef parameterID, Listener* listener);

    ValueTree copyState();
    void replaceState (const ValueTree& newState);

    class SliderAttachment
    {
    public:
        SliderAttachment (AudioProcessorValueTreeState& stateToUse, const String& parameterID, Slider& slider);
        ~SliderAttachment();
    };

    class ButtonAttachment
    {
    public:
        ButtonAttachment (AudioProcessorValueTreeState& stateToUse, const String& parameterID, Button& button);
        ~ButtonAttachment();
    };

    class ComboBoxAttachment
    {
    public:
        ComboBoxAttachment (AudioProcessorValueTreeState& stateToUse, const String& parameterID, ComboBox& combo);
        ~ComboBoxAttachment();
    };

    ValueTree state;
};

class SliderParameterAttachment
{
public:
    SliderParameterAttachment (RangedAudioParameter& parameter, Slider& slider, void* undoManager = nullptr);
    ~SliderParameterAttachment();
};

class WebSliderParameterAttachment
{
public:
    WebSliderParameterAttachment (RangedAudioParameter& parameterIn, WebSliderRelay& sliderStateIn, UndoManager* undoManager = nullptr);
    ~WebSliderParameterAttachment();
};

class WebToggleButtonParameterAttachment
{
public:
    WebToggleButtonParameterAttachment (RangedAudioParameter& parameterIn, WebToggleButtonRelay& button, UndoManager* undoManager = nullptr);
    ~WebToggleButtonParameterAttachment();
};

class WebComboBoxParameterAttachment
{
public:
    WebComboBoxParameterAttachment (RangedAudioParameter& parameterIn, WebComboBoxRelay& combo, UndoManager* undoManager = nullptr);
    ~WebComboBoxParameterAttachment();
};

class TimeSliceClient
{
public:
    virtual ~TimeSliceClient() = default;
    virtual int useTimeSlice() = 0;
};

class TimeSliceThread : public Thread
{
public:
    explicit TimeSliceThread (const String& threadName);
    void addTimeSliceClient (TimeSliceClient* clientToAdd, int millisecondsBeforeStarting = 0);
    void removeTimeSliceClient (TimeSliceClient* clientToRemove);
    void run() override;
};

struct AudioSourceChannelInfo
{
    AudioBuffer<float>* buffer;
    int startSample;
    int numSamples;
};

class AudioSource
{
public:
    virtual ~AudioSource() = default;
    virtual void prepareToPlay (int samplesPerBlockExpected, double sampleRate) = 0;
    virtual void releaseResources() = 0;
    virtual void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) = 0;
};

class AudioIODeviceCallbackContext
{
};

class AudioIODeviceCallback
{
public:
    virtual ~AudioIODeviceCallback() = default;
    virtual void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const AudioIODeviceCallbackContext& context);
};

class AudioDeviceManager
{
public:
    void addAudioCallback (AudioIODeviceCallback* newCallback);
    void removeAudioCallback (AudioIODeviceCallback* callback);
};
class UnitTest
{
public:
    explicit UnitTest (const String& name, const String& category = String());
    virtual ~UnitTest();

    virtual void runTest() = 0;

    void beginTest (const String& testName);
    void expect (bool testResult, const String& failureMessage = String());

    template <class ValueType>
    void expectEquals (ValueType actual, ValueType expected, String failureMessage = String());
    template <class ValueType>
    void expectNotEquals (ValueType value, ValueType valueToCompareTo, String failureMessage = String());
    template <class ValueType>
    void expectGreaterThan (ValueType value, ValueType valueToCompareTo, String failureMessage = String());
    template <class ValueType>
    void expectLessThan (ValueType value, ValueType valueToCompareTo, String failureMessage = String());
    template <class ValueType>
    void expectGreaterOrEqual (ValueType value, ValueType valueToCompareTo, String failureMessage = String());
    template <class ValueType>
    void expectLessOrEqual (ValueType value, ValueType valueToCompareTo, String failureMessage = String());
    template <class ValueType>
    void expectWithinAbsoluteError (ValueType actual, ValueType expected, ValueType maxAbsoluteError, String failureMessage = String());
};

template <typename Type, typename MutexType, bool onlyCreateOncePerRun>
struct SingletonHolder
{
    Type* get();
};
} // namespace juce

#define JUCE_DECLARE_SINGLETON(Classname, doNotRecreateAfterDeletion)                                       \
    static juce::SingletonHolder<Classname, juce::CriticalSection, doNotRecreateAfterDeletion> singletonHolder; \
    static Classname* getInstance() { return singletonHolder.get(); }

#define JUCE_IMPLEMENT_SINGLETON(Classname) \
    decltype (Classname::singletonHolder) Classname::singletonHolder;

#define JUCE_DECLARE_NON_COPYABLE(className) \
    className (const className&) = delete;   \
    className& operator= (const className&) = delete;

#define JUCE_LEAK_DETECTOR(OwnerClass)                                                 \
    friend class juce::LeakedObjectDetector<OwnerClass>;                               \
    static const char* getLeakedObjectClassName() noexcept { return #OwnerClass; }     \
    juce::LeakedObjectDetector<OwnerClass> JUCE_JOIN_MACRO (leakDetector, __LINE__);

#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(className) \
    JUCE_DECLARE_NON_COPYABLE (className)                       \
    JUCE_LEAK_DETECTOR (className)

#define jassertfalse JUCE_BLOCK_WITH_FORCED_SEMICOLON (juce::logAssertion (__FILE__, __LINE__);)
#define jassert(expression) JUCE_BLOCK_WITH_FORCED_SEMICOLON (if (! (expression)) jassertfalse;)

#define DBG(textToWrite) JUCE_BLOCK_WITH_FORCED_SEMICOLON (juce::String tempDbgBuf; tempDbgBuf << textToWrite; juce::Logger::outputDebugString (tempDbgBuf);)
