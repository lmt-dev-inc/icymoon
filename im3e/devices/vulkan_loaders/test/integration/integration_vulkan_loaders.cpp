#include <im3e/devices/vulkan_loaders/vulkan_loaders.h>
#include <im3e/test_utils/test_utils.h>

#include <gmock/gmock.h>

#include <iostream>

using namespace im3e;

TEST(VulkanLoaderIntegration, constructor)
{
    auto pLoader = createVulkanLoader(VulkanLoaderConfig{});
    ASSERT_THAT(pLoader, NotNull());
}

TEST(VulkanLoaderIntegration, constructorWithDebugEnabled)
{
    auto pLoader = createVulkanLoader(VulkanLoaderConfig{
        .isDebugEnabled = true,
    });
    ASSERT_THAT(pLoader, NotNull());
}

TEST(VulkanLoaderIntegration, loadGlobalFcts)
{
    auto pLoader = createVulkanLoader(VulkanLoaderConfig{});
    auto fcts = pLoader->loadGlobalFcts();
    ASSERT_THAT(fcts.vkEnumerateInstanceVersion, NotNull());
    ASSERT_THAT(fcts.vkEnumerateInstanceExtensionProperties, NotNull());
    ASSERT_THAT(fcts.vkEnumerateInstanceLayerProperties, NotNull());
    ASSERT_THAT(fcts.vkCreateInstance, NotNull());

    uint32_t vkVersion{};
    fcts.vkEnumerateInstanceVersion(&vkVersion);
    EXPECT_THAT(VK_API_VERSION_MAJOR(vkVersion), Ge(1U));
    EXPECT_THAT(VK_API_VERSION_MINOR(vkVersion), Ge(3U));
}