#include <juce_mock.h>

#include <optional>

struct Foo
{
    void doSomething();
};

std::unique_ptr<Foo> makeFoo();
std::function<void()> getCallback();
void takesBool(bool);

void smartPointers(std::unique_ptr<Foo>& owned, std::shared_ptr<Foo> shared, std::function<void()>& callback, std::function<void()>& other)
{
    if (owned) // EXPECT_WARNING
        owned->doSomething();

    if (! callback) // EXPECT_WARNING
        return;

    if (shared && callback) // EXPECT_WARNING
        return;

    takesBool(static_cast<bool>(owned));

    const auto isSet = !! callback; // EXPECT_WARNING

    if (true ? callback : other) // EXPECT_WARNING
        return;

    jassert (callback); // EXPECT_WARNING

    (void) isSet;
}

void allowed(Foo* raw, std::optional<int> optional, std::unique_ptr<Foo>& owned)
{
    if (auto foo = makeFoo())
        foo->doSomething();

    if (auto callback = getCallback())
        callback();

    if (owned != nullptr && raw == nullptr)
        return;

    if (raw)
        return;

    if (optional)
        return;
}
