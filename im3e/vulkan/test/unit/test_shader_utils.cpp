#include <im3e/vulkan/shader_utils.h>

#include <im3e/test_utils/test_utils.h>
#include <im3e/vulkan/mock_device.h>

#include <array>
#include <cstdint>
#include <stdexcept>

using namespace im3e;

namespace {

// Smallest code that CompiledShader accepts: a SPIR-V header (magic number, version 1.5, generator, bound, schema).
constexpr std::array<uint32_t, 5U> FakeSpirv{0x07230203U, 0x00010500U, 0U, 1U, 0U};

struct ShaderUtilsTest : public Test
{
    NiceMock<MockDevice> m_mockDevice;
    MockVulkanDeviceFcts& m_mockFcts = m_mockDevice.getMockDeviceFcts();
    const VkDevice m_mockVkDevice = m_mockDevice.getMockVkDevice();
    const CompiledShader m_shader{FakeSpirv, ShaderReflection{}};
};

}  // namespace

TEST_F(ShaderUtilsTest, createVkShaderModule)
{
    const auto mockVkShaderModule = reinterpret_cast<VkShaderModule>(0x3c8e1a);
    EXPECT_CALL(m_mockFcts, vkCreateShaderModule(m_mockVkDevice, NotNull(), IsNull(), NotNull()))
        .WillOnce(Invoke([&](Unused, auto* vkCreateInfo, Unused, auto* vkShaderModule) {
            EXPECT_THAT(vkCreateInfo->sType, Eq(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO));
            EXPECT_THAT(vkCreateInfo->codeSize, Eq(FakeSpirv.size() * sizeof(uint32_t)));  // in bytes, not words
            EXPECT_THAT(vkCreateInfo->pCode, Eq(FakeSpirv.data()));
            *vkShaderModule = mockVkShaderModule;
            return VK_SUCCESS;
        }));

    auto vkShaderModule = createVkShaderModule(m_mockDevice, m_shader);
    EXPECT_THAT(vkShaderModule.get(), Eq(mockVkShaderModule));

    EXPECT_CALL(m_mockFcts, vkDestroyShaderModule(m_mockVkDevice, mockVkShaderModule, IsNull()));
    vkShaderModule.reset();
}

TEST_F(ShaderUtilsTest, createVkShaderModule_onFailure)
{
    ON_CALL(m_mockFcts, vkCreateShaderModule(_, _, _, _)).WillByDefault(Return(VK_ERROR_OUT_OF_HOST_MEMORY));
    EXPECT_THROW(createVkShaderModule(m_mockDevice, m_shader), std::runtime_error);
}
