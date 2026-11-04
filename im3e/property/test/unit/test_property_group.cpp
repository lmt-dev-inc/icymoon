#include <im3e/property/property_group.h>

#include <im3e/property/mock_property_interface.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

auto ConfigEq(const PropertyGroup::Config& expected)
{
    using ConfigT = PropertyGroup::Config;
    return testing::AllOf(Field("name", &ConfigT::name, Eq(expected.name)),
                          Field("description", &ConfigT::description, Eq(expected.description)));
}

auto createMockPropertyNodes(size_t count)
{
    std::vector<std::shared_ptr<MockPropertyNode>> mockPropertyNodes;
    mockPropertyNodes.reserve(count);
    std::generate_n(std::back_inserter(mockPropertyNodes), count,
                    [] { return std::make_shared<NiceMock<MockPropertyNode>>(); });
    return mockPropertyNodes;
}

auto toWeakPtrVector(const std::vector<std::shared_ptr<MockPropertyNode>>& properties)
{
    std::vector<std::weak_ptr<IPropertyNode>> result;
    result.reserve(properties.size());
    std::ranges::transform(properties, std::back_inserter(result), [](const auto& pointer) { return pointer; });
    return result;
}

void expectChildrenRegistered(const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren)
{
    for (const auto& mockChild : mockChildren)
    {
        if (mockChild)
        {
            EXPECT_CALL(*mockChild, registerOnChange(_));
        }
    }
}

void notifyChanged(const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren)
{
    std::ranges::for_each(mockChildren, [](auto& mockChild) { mockChild->notifyChanged(); });
}

void expectCallbackCount(const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren, size_t count)
{
    for (const auto& mockChild : mockChildren)
    {
        EXPECT_THAT(mockChild->getCallbackCount(), Eq(count));
    }
}

}  // namespace

TEST(PropertyGroupTest, constructor)
{
    const PropertyGroup::Config config{.name = "Test Group", .description = "Description"};
    const auto mockChildren = createMockPropertyNodes(5U);
    expectChildrenRegistered(mockChildren);
    PropertyGroup group{config, toWeakPtrVector(mockChildren)};
    EXPECT_THAT(group.getConfig(), ConfigEq(config));
    EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, constructor_withExpiredChildren)
{
    auto mockChildren = createMockPropertyNodes(4U);
    auto weakPtrChildren = toWeakPtrVector(mockChildren);

    // Add a weak_ptr that never pointed to anything:
    weakPtrChildren.emplace_back();

    // Delete children so that some of the weak_ptr are expired:
    mockChildren[1].reset();
    mockChildren[3].reset();

    expectChildrenRegistered(mockChildren);
    PropertyGroup group{{.name = "Group"}, weakPtrChildren};

    // Only 2 mocks are still valid so the group should have cleaned up the given vector:
    EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAre(mockChildren[0], mockChildren[2])));
}

TEST(PropertyGroupTest, constructor_withNoChildren)
{
    PropertyGroup group{{.name = "Empty"}, {}};
    EXPECT_THAT(group.getChildren(), IsEmpty());

    auto interface = group.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    EXPECT_THAT(interface->getChildren(), IsEmpty());
}

TEST(PropertyGroupTest, constructor_withDuplicateChildren)
{
    auto mockChildren = createMockPropertyNodes(3U);
    auto weakPtrChildren = toWeakPtrVector(mockChildren);
    weakPtrChildren.emplace_back(mockChildren[1]);

    EXPECT_THROW((PropertyGroup{{.name = "Group"}, weakPtrChildren}), std::invalid_argument);

    // The registrations made before the duplicate was found are released with the failed group:
    expectCallbackCount(mockChildren, 0U);
}

TEST(PropertyGroupTest, constructor_withOnChangeCallback)
{
    auto mockChildren = createMockPropertyNodes(3U);

    StrictMock<MockFunction<void()>> mockOnChange;
    PropertyGroup group{{.name = "with onChange"}, toWeakPtrVector(mockChildren), mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
    notifyChanged(mockChildren);
}

TEST(PropertyGroupTest, registerOnChange)
{
    auto mockChildren = createMockPropertyNodes(6U);

    PropertyGroup group{{.name = "with onChange"}, toWeakPtrVector(mockChildren)};

    StrictMock<MockFunction<void()>> mockOnChange;
    const auto groupRegistration = group.registerOnChange(mockOnChange.AsStdFunction());

    EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
    notifyChanged(mockChildren);
}

TEST(PropertyGroupTest, registerOnChange_withNullCallback)
{
    PropertyGroup group{{.name = "Test Group"}, {}};
    EXPECT_THROW(auto registration = group.registerOnChange({}), std::invalid_argument);
}

TEST(PropertyGroupTest, registerOnChange_canBeCancelled)
{
    auto mockChildren = createMockPropertyNodes(3U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    MockFunction<void()> mockOnChange;
    auto registration = group.registerOnChange(mockOnChange.AsStdFunction());
    registration.reset();

    EXPECT_CALL(mockOnChange, Call()).Times(0);
    notifyChanged(mockChildren);
}

TEST(PropertyGroupTest, registerOnChange_callbackSeesChildren)
{
    auto mockChildren = createMockPropertyNodes(3U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    MockFunction<void()> mockOnChange;
    const auto registration = group.registerOnChange(mockOnChange.AsStdFunction());

    EXPECT_CALL(mockOnChange, Call()).WillOnce([&] {
        EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
    });
    mockChildren[1]->notifyChanged();
}

TEST(PropertyGroupTest, registerOnChange_afterChildDestroyed)
{
    auto mockChildren = createMockPropertyNodes(3U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    MockFunction<void()> mockOnChange;
    const auto registration = group.registerOnChange(mockOnChange.AsStdFunction());

    // The change of a remaining child makes the group drop the destroyed one before notifying:
    mockChildren.erase(mockChildren.begin() + 1);
    EXPECT_CALL(mockOnChange, Call()).WillOnce([&] {
        EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
    });
    mockChildren.front()->notifyChanged();
    EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, getChildren_whenChildDestroyed)
{
    auto mockChildren = createMockPropertyNodes(5U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    mockChildren.resize(3U);
    EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, getChildren_whenMiddleChildDestroyed)
{
    auto mockChildren = createMockPropertyNodes(4U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    mockChildren.erase(mockChildren.begin() + 1);
    EXPECT_THAT(group.getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, interface)
{
    const auto mockChildren = createMockPropertyNodes(3U);
    PropertyGroup group{
        {
            .name = "Test Group",
            .description = "Test Description",
        },
        toWeakPtrVector(mockChildren),
    };

    auto interface = group.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    EXPECT_THAT(interface->getName(), Eq("Test Group"));
    EXPECT_THAT(interface->getDescription(), Eq("Test Description"));
    EXPECT_THAT(interface->getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, interface_const)
{
    const auto mockChildren = createMockPropertyNodes(3U);
    const PropertyGroup group{{.name = "Test Group"}, toWeakPtrVector(mockChildren)};

    std::shared_ptr<const IPropertyGroup> interface = group.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    EXPECT_THAT(interface->getName(), Eq("Test Group"));
    EXPECT_THAT(interface->getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, interface_registerOnChange)
{
    const auto mockChildren = createMockPropertyNodes(3U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    auto interface = group.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    StrictMock<MockFunction<void()>> mockOnChange;
    const auto registration = interface->registerOnChange(mockOnChange.AsStdFunction());

    EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
    notifyChanged(mockChildren);
}

TEST(PropertyGroupTest, interface_registerOnChange_withNullCallback)
{
    PropertyGroup group{{.name = "Group"}, {}};
    auto interface = group.getInterface().lock();
    ASSERT_THAT(interface, NotNull());
    EXPECT_THROW(auto registration = interface->registerOnChange({}), std::invalid_argument);
}

TEST(PropertyGroupTest, interfaceExpiresWithGroup)
{
    std::weak_ptr<IPropertyGroup> interface;
    {
        PropertyGroup group{{.name = "Group"}, {}};
        interface = group.getInterface();
        EXPECT_THAT(interface.expired(), IsFalse());
    }
    EXPECT_THAT(interface.expired(), IsTrue());
}

TEST(PropertyGroupTest, interfaceOutlivesGroup)
{
    const auto mockChildren = createMockPropertyNodes(3U);
    MockFunction<void()> mockOnChange;
    MockFunction<void()> registeredOnChange;
    CallbackRegistration onChangeRegistration;

    std::shared_ptr<IPropertyGroup> interface;
    {
        PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren), mockOnChange.AsStdFunction()};
        onChangeRegistration = group.registerOnChange(registeredOnChange.AsStdFunction());
        interface = group.getInterface().lock();
        ASSERT_THAT(interface, NotNull());
    }

    // Callbacks fire for as long as their registration is alive. mockOnChange's registration was owned by the group,
    // so it no longer fires. registeredOnChange's registration is owned by this test, so it keeps firing: the
    // interface keeps working while it is referenced.
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    EXPECT_CALL(registeredOnChange, Call()).Times(mockChildren.size());
    notifyChanged(mockChildren);
    EXPECT_THAT(interface->getChildren(), WhenLocked(ElementsAreArray(mockChildren)));
}

TEST(PropertyGroupTest, childOutlivesGroup)
{
    const auto mockChildren = createMockPropertyNodes(3U);
    MockFunction<void()> mockOnChange;
    {
        PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren), mockOnChange.AsStdFunction()};
        expectCallbackCount(mockChildren, 1U);
    }

    // The group released its registrations when destroyed, so the children no longer call into it:
    expectCallbackCount(mockChildren, 0U);
    EXPECT_CALL(mockOnChange, Call()).Times(0);
    notifyChanged(mockChildren);
}