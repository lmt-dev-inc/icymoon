#include <im3e/utils/properties/property_change_notifier.h>

#include <im3e/test_utils/test_utils.h>

#include <gmock/gmock.h>

using namespace im3e;

TEST(PropertyChangeNotifierTest, constructor)
{
    PropertyChangeNotifier notifier;
    notifier.notifyChanged();
}

TEST(PropertyChangeNotifierTest, registerOnChange)
{
    PropertyChangeNotifier notifier;

    bool called{};
    auto callback = [&] { called = true; };
    auto pOnChanged = std::make_shared<std::function<void()>>(callback);
    notifier.registerOnChange(pOnChanged);

    EXPECT_THAT(called, IsFalse());
    notifier.notifyChanged();
    EXPECT_THAT(called, IsTrue());
}

TEST(PropertyChangeNotifierTest, notifyChangedAfterCallbackDeleted)
{
    PropertyChangeNotifier notifier;

    bool called{};
    auto callback = [&] { called = true; };
    auto pOnChanged = std::make_shared<std::function<void()>>(callback);
    notifier.registerOnChange(pOnChanged);

    pOnChanged.reset();

    EXPECT_THAT(called, IsFalse());
    notifier.notifyChanged();
    EXPECT_THAT(called, IsFalse());
}
