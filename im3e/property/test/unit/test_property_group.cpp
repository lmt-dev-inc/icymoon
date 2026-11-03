#include <im3e/property/property_group.h>

#include <im3e/property/mock_property_interface.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

namespace {

auto ConfigEq(const PropertyGroup::Config& expected)
{
    using ConfigT = typename PropertyGroup::Config;
    return testing::AllOf(Field("name", &ConfigT::name, StrEq(expected.name)),
                          Field("description", &ConfigT::description, StrEq(expected.description)));
}

struct PropertyGroupTest : public Test
{
public:
    [[nodiscard]] auto createMockPropertyNodes(size_t count)
    {
        std::vector<std::shared_ptr<MockPropertyNode>> mockPropertyNodes;
        mockPropertyNodes.reserve(count);
        std::generate_n(std::back_inserter(mockPropertyNodes), count,
                        [] { return std::make_shared<NiceMock<MockPropertyNode>>(); });
        return mockPropertyNodes;
    }

    [[nodiscard]] auto toWeakPtrVector(const std::vector<std::shared_ptr<MockPropertyNode>>& properties)
    {
        std::vector<std::weak_ptr<IPropertyNode>> result;
        result.reserve(properties.size());
        std::ranges::transform(properties, std::back_inserter(result), [](const auto& pointer) { return pointer; });
        return result;
    }

    void expectChildrenRegistered(const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren)
    {
        for (auto& mockChild : mockChildren)
        {
            if (mockChild)
            {
                EXPECT_CALL(*mockChild, registerOnChange(_));
            }
        }
    }

    void expectChildren(const PropertyGroup& group, const std::vector<std::shared_ptr<MockPropertyNode>>& mockChildren)
    {
        EXPECT_THAT(
            std::ranges::equal(group.getChildren(), mockChildren, [](auto& a, auto& b) { return a.lock() == b; }),
            IsTrue());
    }
};

}  // namespace

TEST_F(PropertyGroupTest, constructor)
{
    const PropertyGroup::Config config{.name = "Test Group", .description = "Description"};
    const auto mockChildren = createMockPropertyNodes(5U);
    expectChildrenRegistered(mockChildren);
    PropertyGroup group{config, toWeakPtrVector(mockChildren)};
    EXPECT_THAT(group.getConfig(), ConfigEq(config));
    expectChildren(group, mockChildren);
}

TEST_F(PropertyGroupTest, constructor_withNullChildren)
{
    auto mockChildren = createMockPropertyNodes(4U);
    auto weakPtrChildren = toWeakPtrVector(mockChildren);

    // Delete children so that some of the weak_ptr are empty:
    mockChildren[1].reset();
    mockChildren[3].reset();

    expectChildrenRegistered(mockChildren);
    PropertyGroup group{{.name = "Group"}, weakPtrChildren};

    // Only 2 mocks are still valid so the group should have cleaned up the given vector:
    expectChildren(group, {mockChildren[0], mockChildren[2]});
}

TEST_F(PropertyGroupTest, constructor_withOnChangeCallback)
{
    auto mockChildren = createMockPropertyNodes(3U);

    StrictMock<MockFunction<void()>> mockOnChange;
    PropertyGroup group{{.name = "with onChange"}, toWeakPtrVector(mockChildren), mockOnChange.AsStdFunction()};

    EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
    for (auto& mockChild : mockChildren)
    {
        const auto& registeredCallbacks = mockChild->getRegisteredOnChange();
        EXPECT_THAT(registeredCallbacks.size(), Eq(1U));
        const auto& onChange = registeredCallbacks.front();
        EXPECT_THAT(!!onChange, IsTrue());
        onChange();
    }
}

TEST_F(PropertyGroupTest, registerOnChange)
{
    auto mockChildren = createMockPropertyNodes(6U);

    PropertyGroup group{{.name = "with onChange"}, toWeakPtrVector(mockChildren)};

    StrictMock<MockFunction<void()>> mockOnChange;
    const auto groupRegistration = group.registerOnChange(mockOnChange.AsStdFunction());

    EXPECT_CALL(mockOnChange, Call()).Times(mockChildren.size());
    for (auto& mockChild : mockChildren)
    {
        const auto& registeredCallbacks = mockChild->getRegisteredOnChange();
        EXPECT_THAT(registeredCallbacks.size(), Eq(1U));
        const auto& onChange = registeredCallbacks.front();
        EXPECT_THAT(!!onChange, IsTrue());
        onChange();
    }
}

TEST_F(PropertyGroupTest, registerOnChange_withNullCallback)
{
    PropertyGroup group{{.name = "Test Group"}, {}};
    EXPECT_THROW(auto registration = group.registerOnChange({}), std::invalid_argument);
}

TEST_F(PropertyGroupTest, getChildren_whenChildDestroyed)
{
    auto mockChildren = createMockPropertyNodes(5U);
    PropertyGroup group{{.name = "Group"}, toWeakPtrVector(mockChildren)};

    mockChildren.resize(3U);
    expectChildren(group, mockChildren);
}