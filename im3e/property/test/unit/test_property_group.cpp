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

auto LockedElementsAre(const std::vector<std::shared_ptr<MockPropertyNode>>& expected)
{
    return ResultOf(
        "locked children",
        [](const std::vector<std::weak_ptr<IPropertyNode>>& children) {
            std::vector<std::shared_ptr<IPropertyNode>> locked;
            locked.reserve(children.size());
            std::ranges::transform(children, std::back_inserter(locked),
                                   [](const auto& child) { return child.lock(); });
            return locked;
        },
        ElementsAreArray(expected));
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

}  // namespace

TEST(PropertyGroupTest, constructor)
{
    const PropertyGroup::Config config{.name = "Test Group", .description = "Description"};
    const auto mockChildren = createMockPropertyNodes(5U);
    expectChildrenRegistered(mockChildren);
    PropertyGroup group{config, toWeakPtrVector(mockChildren)};
    EXPECT_THAT(group.getConfig(), ConfigEq(config));
    EXPECT_THAT(group.getChildren(), LockedElementsAre(mockChildren));
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
    EXPECT_THAT(group.getChildren(), LockedElementsAre({mockChildren[0], mockChildren[2]}));
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

TEST(PropertyGroupTest, getChildren_whenChildDestroyed)
{
    auto mockChildren = createMockPropertyNodes(5U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    mockChildren.resize(3U);
    EXPECT_THAT(group.getChildren(), LockedElementsAre(mockChildren));
}
