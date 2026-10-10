#include <im3e/utils/vk_utils.h>

#include <im3e/test_utils/test_utils.h>

#include <fmt/format.h>

using namespace im3e;

TEST(VkUtilsTest, VkExtent2DEqOperator)
{
    const VkExtent2D vkExtent1{.width = 1920U, .height = 1080U};
    const VkExtent2D vkExtent2{.width = 1080U, .height = 1080U};
    const VkExtent2D vkExtent3{.width = 1920U, .height = 1920U};
    const VkExtent2D vkExtent4{.width = 3840U, .height = 2160U};
    EXPECT_THAT(vkExtent1 == vkExtent1, IsTrue());
    EXPECT_THAT(vkExtent1 == vkExtent2, IsFalse());
    EXPECT_THAT(vkExtent1 == vkExtent3, IsFalse());
    EXPECT_THAT(vkExtent1 == vkExtent4, IsFalse());
}

TEST(VkUtilsTest, VkExtent2DDiffOperator)
{
    const VkExtent2D vkExtent1{.width = 1920U, .height = 1080U};
    const VkExtent2D vkExtent2{.width = 1080U, .height = 1080U};
    const VkExtent2D vkExtent3{.width = 1920U, .height = 1920U};
    const VkExtent2D vkExtent4{.width = 3840U, .height = 2160U};
    EXPECT_THAT(vkExtent1 != vkExtent1, IsFalse());
    EXPECT_THAT(vkExtent1 != vkExtent2, IsTrue());
    EXPECT_THAT(vkExtent1 != vkExtent3, IsTrue());
    EXPECT_THAT(vkExtent1 != vkExtent4, IsTrue());
}

TEST(VkUtilsTest, makeVkUniquePtr)
{
    const auto mockVkDevice = reinterpret_cast<VkDevice>(0x34ef5a);
    const auto mockVkImage = reinterpret_cast<VkImage>(0x23e45d);
    MockFunction<void(VkDevice, VkImage, const VkAllocationCallbacks*)> mockDestructor;

    auto pUniquePtr = makeVkUniquePtr<VkImage>(mockVkDevice, mockVkImage, mockDestructor.AsStdFunction());

    EXPECT_CALL(mockDestructor, Call(mockVkDevice, mockVkImage, IsNull()));
    pUniquePtr.reset();
}

TEST(VkUtilsTest, throwIfVkFailedThrowsOnFailure)
{
    EXPECT_THROW(throwIfVkFailed(VK_ERROR_UNKNOWN, "Expected Failure"), std::runtime_error);
}

TEST(VkUtilsTest, throwIfVkFailedDoesNotThrowOnSuccess)
{
    EXPECT_NO_THROW(throwIfVkFailed(VK_SUCCESS, "Unexpected Failure"));
}

TEST(VkUtilsTest, getVkList)
{
    const VkDevice vkDevice = reinterpret_cast<VkDevice>(0xc0ffee);
    const char* pName = "Hello";
    const uint32_t expectedCount = 4U;
    const std::vector<float> expectedValues{0.0F, 1.0F, 2.0F, 3.0F};

    MockFunction<VkResult(VkDevice, const char* pName, uint32_t* pCount, float* pValues)> mockVkFct;
    {
        InSequence s;
        EXPECT_CALL(mockVkFct, Call(vkDevice, StrEq(pName), NotNull(), IsNull()))
            .WillOnce(Invoke([&](Unused, Unused, uint32_t* pCount, Unused) {
                *pCount = expectedCount;
                return VK_SUCCESS;
            }));
        EXPECT_CALL(mockVkFct, Call(vkDevice, StrEq(pName), NotNull(), NotNull()))
            .WillOnce(Invoke([&](Unused, Unused, uint32_t* pCount, float* pValues) {
                EXPECT_THAT(*pCount, Eq(expectedCount));
                std::ranges::copy(expectedValues, pValues);
                return VK_SUCCESS;
            }));
    }

    const auto actualValues = getVkList<float>(mockVkFct.AsStdFunction(), "mockVkFct", vkDevice, pName);
    EXPECT_THAT(actualValues, ContainerEq(expectedValues));
}

TEST(VkUtilsTest, vkFlagsContain)
{
    EXPECT_THAT(vkFlagsContain(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
                IsTrue());
    EXPECT_THAT(
        vkFlagsContain(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT),
        IsFalse());
}

TEST(VkUtilsTest, getFormatProperties)
{
    auto testFormatProperties = [](VkFormat vkFormat, const FormatProperties& rExpected) {
        const auto properties = getFormatProperties(vkFormat);
        const std::string errorMessage = fmt::format("VkFormat = {}", static_cast<uint32_t>(vkFormat));
        EXPECT_THAT(properties.sizeInBytes, Eq(rExpected.sizeInBytes)) << errorMessage;
        EXPECT_THAT(properties.componentSizeInBytes, Eq(rExpected.componentSizeInBytes)) << errorMessage;
        EXPECT_THAT(properties.componentCount, Eq(rExpected.componentCount)) << errorMessage;
    };

    const FormatProperties props3Ch1B{.sizeInBytes = 3U, .componentSizeInBytes = 1U, .componentCount = 3U};
    testFormatProperties(VK_FORMAT_B8G8R8_SRGB, props3Ch1B);
    testFormatProperties(VK_FORMAT_B8G8R8_UNORM, props3Ch1B);
    testFormatProperties(VK_FORMAT_B8G8R8_SNORM, props3Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8_SRGB, props3Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8_UNORM, props3Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8_SNORM, props3Ch1B);

    const FormatProperties props4Ch1B{.sizeInBytes = 4U, .componentSizeInBytes = 1U, .componentCount = 4U};
    testFormatProperties(VK_FORMAT_B8G8R8A8_SRGB, props4Ch1B);
    testFormatProperties(VK_FORMAT_B8G8R8A8_UNORM, props4Ch1B);
    testFormatProperties(VK_FORMAT_B8G8R8A8_SNORM, props4Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8A8_SRGB, props4Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8A8_UNORM, props4Ch1B);
    testFormatProperties(VK_FORMAT_R8G8B8A8_SNORM, props4Ch1B);

    const FormatProperties props2Ch4B{.sizeInBytes = 8U, .componentSizeInBytes = 4U, .componentCount = 2U};
    testFormatProperties(VK_FORMAT_R32G32_SFLOAT, props2Ch4B);

    const FormatProperties props3Ch4B{.sizeInBytes = 12U, .componentSizeInBytes = 4U, .componentCount = 3U};
    testFormatProperties(VK_FORMAT_R32G32B32_SFLOAT, props3Ch4B);

    const FormatProperties props4Ch4B{.sizeInBytes = 16U, .componentSizeInBytes = 4U, .componentCount = 4U};
    testFormatProperties(VK_FORMAT_R32G32B32A32_SFLOAT, props4Ch4B);

    const FormatProperties propsCompressed{.sizeInBytes = 0U, .componentSizeInBytes = 0U, .componentCount = 0U};
    testFormatProperties(VK_FORMAT_BC1_RGB_SRGB_BLOCK, propsCompressed);
}

TEST(VkUtilsTest, getFormatPropertiesThrowsWhenNotSupported)
{
    EXPECT_THROW(getFormatProperties(VK_FORMAT_UNDEFINED), std::invalid_argument);
}

TEST(VkUtilsTest, toVkExtent3D)
{
    VkExtent2D vkExtent2d{.width = 800U, .height = 600U};
    const auto vkExtent3d = toVkExtent3D(vkExtent2d);
    EXPECT_THAT(vkExtent3d.width, Eq(vkExtent2d.width));
    EXPECT_THAT(vkExtent3d.height, Eq(vkExtent2d.height));
    EXPECT_THAT(vkExtent3d.depth, Eq(1U));
}