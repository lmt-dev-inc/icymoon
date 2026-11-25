#include <im3e/vulkan/sync_utils.h>

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/mock_device.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace im3e;

namespace {

struct SyncUtilsTest : public Test
{
    NiceMock<MockDevice> m_mockDevice;
    MockVulkanDeviceFcts& m_mockFcts = m_mockDevice.getMockDeviceFcts();
    const VkDevice m_mockVkDevice = m_mockDevice.getMockVkDevice();
};

}  // namespace

TEST_F(SyncUtilsTest, createVkSemaphore)
{
    const auto mockVkSemaphore = reinterpret_cast<VkSemaphore>(0x5e3a4f);
    EXPECT_CALL(m_mockFcts, vkCreateSemaphore(m_mockVkDevice, NotNull(), IsNull(), NotNull()))
        .WillOnce(Invoke([&](Unused, auto* vkCreateInfo, Unused, auto* vkSemaphore) {
            EXPECT_THAT(vkCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO));
            *vkSemaphore = mockVkSemaphore;
            return VK_SUCCESS;
        }));

    auto vkSemaphore = createVkSemaphore(m_mockDevice);
    EXPECT_THAT(vkSemaphore.get(), Eq(mockVkSemaphore));

    EXPECT_CALL(m_mockFcts, vkDestroySemaphore(m_mockVkDevice, mockVkSemaphore, IsNull()));
    vkSemaphore.reset();
}

TEST_F(SyncUtilsTest, createVkSemaphore_onFailure)
{
    ON_CALL(m_mockFcts, vkCreateSemaphore(_, _, _, _)).WillByDefault(Return(VK_ERROR_OUT_OF_DEVICE_MEMORY));
    EXPECT_THROW(createVkSemaphore(m_mockDevice), std::runtime_error);
}

TEST_F(SyncUtilsTest, createVkFence)
{
    const auto mockVkFence = reinterpret_cast<VkFence>(0x4f7e1c);
    EXPECT_CALL(m_mockFcts, vkCreateFence(m_mockVkDevice, NotNull(), IsNull(), NotNull()))
        .WillOnce(Invoke([&](Unused, auto* vkCreateInfo, Unused, auto* vkFence) {
            EXPECT_THAT(vkCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO));
            EXPECT_THAT(vkCreateInfo->flags, Eq(VkFenceCreateFlags{VK_FENCE_CREATE_SIGNALED_BIT}));
            *vkFence = mockVkFence;
            return VK_SUCCESS;
        }));

    auto vkFence = createVkFence(m_mockDevice, VK_FENCE_CREATE_SIGNALED_BIT);
    EXPECT_THAT(vkFence.get(), Eq(mockVkFence));

    EXPECT_CALL(m_mockFcts, vkDestroyFence(m_mockVkDevice, mockVkFence, IsNull()));
    vkFence.reset();
}

TEST_F(SyncUtilsTest, createVkFence_withoutFlags)
{
    EXPECT_CALL(m_mockFcts,
                vkCreateFence(m_mockVkDevice, Pointee(Field(&VkFenceCreateInfo::flags, VkFenceCreateFlags{})), IsNull(),
                              NotNull()));
    createVkFence(m_mockDevice);
}

TEST_F(SyncUtilsTest, createVkFence_onFailure)
{
    ON_CALL(m_mockFcts, vkCreateFence(_, _, _, _)).WillByDefault(Return(VK_ERROR_OUT_OF_DEVICE_MEMORY));
    EXPECT_THROW(createVkFence(m_mockDevice), std::runtime_error);
}

TEST_F(SyncUtilsTest, waitForVkFence)
{
    const auto mockVkFence = reinterpret_cast<VkFence>(0x2b9d3e);
    EXPECT_CALL(m_mockFcts, vkWaitForFences(m_mockVkDevice, 1U, Pointee(mockVkFence), VK_TRUE,
                                            std::numeric_limits<uint64_t>::max()))
        .WillOnce(Return(VK_SUCCESS));

    waitForVkFence(m_mockDevice, mockVkFence);
}

TEST_F(SyncUtilsTest, waitForVkFence_withoutFence)
{
    EXPECT_CALL(m_mockFcts, vkWaitForFences(_, _, _, _, _)).Times(0);
    waitForVkFence(m_mockDevice, VK_NULL_HANDLE);
}

TEST_F(SyncUtilsTest, waitForVkFence_onFailure)
{
    const auto mockVkFence = reinterpret_cast<VkFence>(0x2b9d3e);
    ON_CALL(m_mockFcts, vkWaitForFences(_, _, _, _, _)).WillByDefault(Return(VK_ERROR_DEVICE_LOST));
    EXPECT_THROW(waitForVkFence(m_mockDevice, mockVkFence), std::runtime_error);
}
