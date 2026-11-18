#include "vulkan_physical_devices.h"

#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/mock/mock_logger.h>
#include <im3e/vulkan/mock_vulkan_functions.h>
#include <im3e/vulkan/test/mock_vulkan_extensions.h>
#include <im3e/vulkan/test/mock_vulkan_helper.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace im3e;

namespace {

/// @brief Physical device served by the mocked Vulkan functions of VulkanPhysicalDevicesTest.
struct FakePhysicalDevice
{
    VkPhysicalDevice vkPhysicalDevice{};
    VkPhysicalDeviceProperties vkProperties{};
    std::vector<VkQueueFamilyProperties> vkQueueFamilies;
    std::vector<VkExtensionProperties> vkExtensions;
};

auto createQueueFamily(VkQueueFlags vkQueueFlags) -> VkQueueFamilyProperties
{
    return VkQueueFamilyProperties{
        .queueFlags = vkQueueFlags,
        .queueCount = 1U,
    };
}

/// @brief Create a device meeting all requirements: Vulkan 1.3 discrete GPU, with one queue family supporting
/// graphics, compute and transfer. Tests change the fields they are about.
/// @param id Non-zero identifier, used as the device handle and ID.
auto createValidDevice(uint32_t id, std::string_view name) -> FakePhysicalDevice
{
    FakePhysicalDevice device{
        .vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(static_cast<uintptr_t>(id)),
        .vkProperties{
            .apiVersion = VK_API_VERSION_1_3,
            .deviceID = id,
            .deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        },
        .vkQueueFamilies{
            createQueueFamily(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT),
        },
    };
    name.copy(&device.vkProperties.deviceName[0], VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1U);
    return device;
}

/// @brief Match a VulkanPhysicalDevice created from the given fake device.
auto IsFakeDevice(const FakePhysicalDevice& device) -> ::testing::Matcher<const VulkanPhysicalDevice&>
{
    return Field("vkPhysicalDevice", &VulkanPhysicalDevice::vkPhysicalDevice, Eq(device.vkPhysicalDevice));
}

/// @brief Queue family indices expected by HasQueueFamilies(). Omitted fields expect no family.
struct ExpectedQueueFamilies
{
    std::vector<uint32_t> graphics;
    std::vector<uint32_t> compute;
    std::vector<uint32_t> transfer;
    std::vector<uint32_t> presentation;
};

/// @brief Match the queue family indices of a VulkanPhysicalDevice.
auto HasQueueFamilies(const ExpectedQueueFamilies& expected) -> ::testing::Matcher<const VulkanPhysicalDevice&>
{
    using QueueFamilies = VulkanPhysicalDevice::QueueFamilies;
    return Field(
        "queueFamilies", &VulkanPhysicalDevice::queueFamilies,
        AllOf(Field("graphicsFamilyIndices", &QueueFamilies::graphicsFamilyIndices, ContainerEq(expected.graphics)),
              Field("computeFamilyIndices", &QueueFamilies::computeFamilyIndices, ContainerEq(expected.compute)),
              Field("transferFamilyIndices", &QueueFamilies::transferFamilyIndices, ContainerEq(expected.transfer)),
              Field("presentationFamilyIndices", &QueueFamilies::presentationFamilyIndices,
                    ContainerEq(expected.presentation))));
}

struct VulkanPhysicalDevicesTest : public Test
{
    VulkanPhysicalDevicesTest()
    {
        ON_CALL(m_mockExtensions, getDeviceExtensions()).WillByDefault(ReturnRef(m_requiredDeviceExtensions));
        ON_CALL(m_mockIsPresentationSupported, Call(_, _, _)).WillByDefault(Return(true));

        auto& mockFcts = m_mockVk.getMockInstanceFcts();
        ON_CALL(mockFcts, vkEnumeratePhysicalDevices(m_vkInstance, NotNull(), _))
            .WillByDefault(Invoke([this](Unused, uint32_t* count, VkPhysicalDevice* vkPhysicalDevices) {
                std::vector<VkPhysicalDevice> handles;
                std::ranges::transform(m_devices, std::back_inserter(handles), &FakePhysicalDevice::vkPhysicalDevice);
                return fillVkList(handles, count, vkPhysicalDevices);
            }));
        ON_CALL(mockFcts, vkGetPhysicalDeviceProperties(_, NotNull()))
            .WillByDefault(Invoke([this](VkPhysicalDevice vkPhysicalDevice, VkPhysicalDeviceProperties* vkProperties) {
                *vkProperties = findDevice(vkPhysicalDevice).vkProperties;
            }));
        ON_CALL(mockFcts, vkGetPhysicalDeviceQueueFamilyProperties(_, NotNull(), _))
            .WillByDefault(Invoke(
                [this](VkPhysicalDevice vkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* vkQueueFamilies) {
                    fillVkList(findDevice(vkPhysicalDevice).vkQueueFamilies, count, vkQueueFamilies);
                }));
        ON_CALL(mockFcts, vkEnumerateDeviceExtensionProperties(_, IsNull(), NotNull(), _))
            .WillByDefault(Invoke([this](VkPhysicalDevice vkPhysicalDevice, Unused, uint32_t* count,
                                         VkExtensionProperties* vkExtensions) {
                return fillVkList(findDevice(vkPhysicalDevice).vkExtensions, count, vkExtensions);
            }));
    }

    /// @brief Create VulkanPhysicalDevices with the given devices served by the mocked Vulkan functions.
    /// @details Presentation is required, and supported by default (see m_mockIsPresentationSupported).
    auto createDevices(std::vector<FakePhysicalDevice> devices) -> VulkanPhysicalDevices
    {
        return createDevices(std::move(devices), m_mockIsPresentationSupported.AsStdFunction());
    }

    /// @brief Create VulkanPhysicalDevices with the given devices, and without a presentation support function, so
    /// that presentation isn't required.
    auto createDevicesWithoutPresentation(std::vector<FakePhysicalDevice> devices) -> VulkanPhysicalDevices
    {
        return createDevices(std::move(devices), IsPresentationSupportedFct{});
    }

    auto createDevices(std::vector<FakePhysicalDevice> devices,
                       const IsPresentationSupportedFct& isPresentationSupported) -> VulkanPhysicalDevices
    {
        m_devices = std::move(devices);
        return VulkanPhysicalDevices(m_mockLogger, m_mockVk.getInstanceFcts(), m_mockExtensions, m_vkInstance,
                                     isPresentationSupported);
    }

    /// @brief Make the presentation support function return false for all queue families of the given device.
    void disablePresentation(const FakePhysicalDevice& device)
    {
        ON_CALL(m_mockIsPresentationSupported, Call(_, device.vkPhysicalDevice, _)).WillByDefault(Return(false));
    }

    auto findDevice(VkPhysicalDevice vkPhysicalDevice) const -> const FakePhysicalDevice&
    {
        const auto itFind = std::ranges::find(m_devices, vkPhysicalDevice, &FakePhysicalDevice::vkPhysicalDevice);
        if (itFind == m_devices.end())
        {
            throw std::out_of_range("Unknown fake physical device");
        }
        return *itFind;
    }

    NiceMock<MockLogger> m_mockLogger;
    NiceMock<MockVulkanFunctions> m_mockVk;
    NiceMock<MockVulkanExtensions> m_mockExtensions;
    NiceMock<MockFunction<bool(VkInstance, VkPhysicalDevice, uint32_t)>> m_mockIsPresentationSupported;

    VkInstance m_vkInstance = reinterpret_cast<VkInstance>(0x12345);
    std::vector<const char*> m_requiredDeviceExtensions;
    std::vector<FakePhysicalDevice> m_devices;
};

}  // namespace

TEST_F(VulkanPhysicalDevicesTest, constructor_withoutVkInstance)
{
    EXPECT_THROW(VulkanPhysicalDevices(m_mockLogger, m_mockVk.getInstanceFcts(), m_mockExtensions, VK_NULL_HANDLE,
                                       m_mockIsPresentationSupported.AsStdFunction()),
                 std::invalid_argument);
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withValidDevice)
{
    const auto device = createValidDevice(1U, "Valid device");
    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), AllOf(IsFakeDevice(device), HasQueueFamilies({
                                                                                .graphics = {0U},
                                                                                .compute = {0U},
                                                                                .transfer = {0U},
                                                                                .presentation = {0U},
                                                                            })));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withNoDevice)
{
    EXPECT_THROW(createDevices({}), std::runtime_error);
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withOldDevice)
{
    const auto device13 = createValidDevice(1U, "Vulkan 1.3 device");
    auto device12 = createValidDevice(2U, "Vulkan 1.2 device");
    device12.vkProperties.apiVersion = VK_API_VERSION_1_2;

    // Both devices have the same score, so the last one would be chosen if its version weren't checked:
    const auto devices = createDevices({device13, device12});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(device13));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withoutPresentationFct)
{
    const auto device = createValidDevice(1U, "Valid device");
    const auto devices = createDevicesWithoutPresentation({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), AllOf(IsFakeDevice(device), HasQueueFamilies({
                                                                                .graphics = {0U},
                                                                                .compute = {0U},
                                                                                .transfer = {0U},
                                                                            })));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceNotSupportingPresentation)
{
    auto discreteDevice = createValidDevice(1U, "Discrete device without presentation");
    discreteDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    disablePresentation(discreteDevice);

    auto integratedDevice = createValidDevice(2U, "Integrated device with presentation");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    const auto devices = createDevices({discreteDevice, integratedDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withNoDeviceSupportingPresentation)
{
    const auto device1 = createValidDevice(1U, "Device 1");
    disablePresentation(device1);
    const auto device2 = createValidDevice(2U, "Device 2");
    disablePresentation(device2);

    EXPECT_THROW(createDevices({device1, device2}), std::runtime_error);
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withPresentationOnSomeQueueFamilies)
{
    auto device = createValidDevice(1U, "Valid device");
    device.vkQueueFamilies = {
        createQueueFamily(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT),
        createQueueFamily(VK_QUEUE_TRANSFER_BIT),
    };
    ON_CALL(m_mockIsPresentationSupported, Call(m_vkInstance, device.vkPhysicalDevice, 1U))
        .WillByDefault(Return(false));

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), HasQueueFamilies({
                                                    .graphics = {0U},
                                                    .compute = {0U},
                                                    .transfer = {0U, 1U},
                                                    .presentation = {0U},
                                                }));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_prefersDiscreteGpu)
{
    auto discreteDevice = createValidDevice(1U, "Discrete GPU");
    discreteDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    auto integratedDevice = createValidDevice(2U, "Integrated GPU");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    auto virtualDevice = createValidDevice(3U, "Virtual GPU");
    virtualDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    auto cpuDevice = createValidDevice(4U, "CPU");
    cpuDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;

    // The preferred device is in the middle of the list, so that it isn't chosen for being the first or last one:
    const auto devices = createDevices({integratedDevice, virtualDevice, discreteDevice, cpuDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(discreteDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_prefersIntegratedGpuWithoutDiscreteGpu)
{
    auto integratedDevice = createValidDevice(1U, "Integrated GPU");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    auto virtualDevice = createValidDevice(2U, "Virtual GPU");
    virtualDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    auto cpuDevice = createValidDevice(3U, "CPU");
    cpuDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;

    const auto devices = createDevices({virtualDevice, integratedDevice, cpuDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withVirtualGpu)
{
    auto device = createValidDevice(1U, "Virtual GPU");
    device.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(device));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withCpu)
{
    auto device = createValidDevice(1U, "CPU");
    device.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(device));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withUnknownDeviceType)
{
    auto unknownDevice = createValidDevice(1U, "Unknown device type");
    unknownDevice.vkProperties.deviceType = static_cast<VkPhysicalDeviceType>(1000);

    const auto devices = createDevices({unknownDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(unknownDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_prefersIntegratedGpuOverUnknownDeviceType)
{
    auto integratedDevice = createValidDevice(1U, "Integrated GPU");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    auto unknownDevice = createValidDevice(2U, "Unknown device type");
    unknownDevice.vkProperties.deviceType = static_cast<VkPhysicalDeviceType>(1000);

    const auto devices = createDevices({integratedDevice, unknownDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withRequiredExtensions)
{
    m_requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME};
    auto device = createValidDevice(1U, "Device with extensions");
    device.vkExtensions = {
        VkExtensionProperties{.extensionName = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, .specVersion = 1U},
        VkExtensionProperties{.extensionName = VK_KHR_RAY_QUERY_EXTENSION_NAME, .specVersion = 1U},
        VkExtensionProperties{.extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME, .specVersion = 1U},
    };

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(device));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceMissingRequiredExtensions)
{
    m_requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME};
    const std::vector<VkExtensionProperties> allExtensions{
        VkExtensionProperties{.extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME, .specVersion = 1U},
        VkExtensionProperties{.extensionName = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, .specVersion = 1U},
    };

    auto discreteDevice = createValidDevice(1U, "Discrete GPU with some extensions");
    discreteDevice.vkExtensions = {allExtensions[0]};
    auto integratedDevice = createValidDevice(2U, "Integrated GPU with all extensions");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    integratedDevice.vkExtensions = allExtensions;

    const auto devices = createDevices({discreteDevice, integratedDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withNoDeviceSupportingRequiredExtensions)
{
    m_requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const auto device1 = createValidDevice(1U, "Device 1");
    const auto device2 = createValidDevice(2U, "Device 2");

    EXPECT_THROW(createDevices({device1, device2}), std::runtime_error);
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withSeparateQueueFamilies)
{
    auto device = createValidDevice(1U, "Device with separate queue families");
    device.vkQueueFamilies = {
        createQueueFamily(VK_QUEUE_GRAPHICS_BIT),
        createQueueFamily(VK_QUEUE_COMPUTE_BIT),
        createQueueFamily(VK_QUEUE_TRANSFER_BIT),
    };

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), AllOf(IsFakeDevice(device), HasQueueFamilies({
                                                                                .graphics = {0U},
                                                                                .compute = {1U},
                                                                                .transfer = {2U},
                                                                                .presentation = {0U, 1U, 2U},
                                                                            })));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceMissingGraphicsQueue)
{
    auto discreteDevice = createValidDevice(1U, "Discrete GPU without graphics");
    discreteDevice.vkQueueFamilies = {createQueueFamily(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)};
    auto integratedDevice = createValidDevice(2U, "Integrated GPU");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    const auto devices = createDevices({discreteDevice, integratedDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceMissingTransferQueue)
{
    auto discreteDevice = createValidDevice(1U, "Discrete GPU without transfer");
    discreteDevice.vkQueueFamilies = {createQueueFamily(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)};
    auto integratedDevice = createValidDevice(2U, "Integrated GPU");
    integratedDevice.vkProperties.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    const auto devices = createDevices({discreteDevice, integratedDevice});

    EXPECT_THAT(devices.choosePhysicalDevice(), IsFakeDevice(integratedDevice));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceMissingComputeQueue)
{
    // Compute support isn't required:
    auto device = createValidDevice(1U, "Device without compute");
    device.vkQueueFamilies = {createQueueFamily(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)};

    const auto devices = createDevices({device});

    EXPECT_THAT(devices.choosePhysicalDevice(), AllOf(IsFakeDevice(device), HasQueueFamilies({
                                                                                .graphics = {0U},
                                                                                .transfer = {0U},
                                                                                .presentation = {0U},
                                                                            })));
}

TEST_F(VulkanPhysicalDevicesTest, constructor_withDeviceWithoutQueueFamilies)
{
    auto device = createValidDevice(1U, "Device without queue families");
    device.vkQueueFamilies.clear();

    EXPECT_THROW(createDevices({device}), std::runtime_error);
}
