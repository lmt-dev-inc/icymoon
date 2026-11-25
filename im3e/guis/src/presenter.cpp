#include "presenter.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/vulkan/sync_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>

using namespace im3e;

namespace {

auto getVkSurfaceCapabilities(const IDevice& device, VkSurfaceKHR vkSurface) -> VkSurfaceCapabilitiesKHR
{
    const auto& instFcts = device.getInstanceFcts();
    const auto vkPhysicalDevice = device.getVkPhysicalDevice();

    VkSurfaceCapabilitiesKHR vkSurfaceCapabilities{};
    throwIfVkFailed(
        instFcts.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &vkSurfaceCapabilities),
        "Could not query device surface capabilities for Presenter");
    return vkSurfaceCapabilities;
}

auto determineImageCount(const VkSurfaceCapabilitiesKHR& vkCapabilities) -> uint32_t
{
    constexpr auto TargetImageCount = 2U;

    const auto imageCount = std::max(vkCapabilities.minImageCount, TargetImageCount);
    return vkCapabilities.maxImageCount == 0 ? imageCount : std::min(imageCount, vkCapabilities.maxImageCount);
}

auto chooseSurfaceFormat(const ILogger& logger, const std::vector<VkSurfaceFormatKHR>& vkSurfaceFormats)
    -> VkSurfaceFormatKHR
{
    auto itFind = std::ranges::find_if(vkSurfaceFormats, [](const auto& vkSurfaceFormat) {
        return vkSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
               vkSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });

    if (itFind != vkSurfaceFormats.end())
    {
        logger.info("Preferred format VK_FORMAT_B8G8R8A8_SRGB is supported");
        return *itFind;
    }

    throwIfFalse<std::invalid_argument>(!vkSurfaceFormats.empty(), "No surface format is supported");
    logger.warning("Could not find preferred format, falling back to first available format");
    return vkSurfaceFormats.front();
}

auto chooseExtent(const ILogger& logger, const VkSurfaceCapabilitiesKHR& vkCapabilities) -> VkExtent2D
{
    const auto& vkCurrentExtent = vkCapabilities.currentExtent;
    const auto& vkMaxExtent = vkCapabilities.maxImageExtent;
    const auto maxValue = std::numeric_limits<uint32_t>::max();

    VkExtent2D vkExtent{
        .width = (vkCurrentExtent.width == maxValue) ? vkMaxExtent.width : vkCurrentExtent.width,
        .height = (vkCurrentExtent.height == maxValue) ? vkMaxExtent.height : vkCurrentExtent.height,
    };
    logger.info(fmt::format("Choosing extent {}x{} (max: {}x{})", vkExtent.width, vkExtent.height, vkMaxExtent.width,
                            vkMaxExtent.height));
    return vkExtent;
}

auto createVkSwapchainCreateInfo(const ILogger& logger, const IDevice& device, VkSurfaceKHR vkSurface)
    -> VkSwapchainCreateInfoKHR
{
    throwIfNull<std::invalid_argument>(vkSurface, "WindowPresenter requires a surface");

    const auto& instFcts = device.getInstanceFcts();
    const auto vkPhysicalDevice = device.getVkPhysicalDevice();
    const auto vkSurfaceCapabilities = getVkSurfaceCapabilities(device, vkSurface);
    const auto vkSurfaceFormats = getVkList<VkSurfaceFormatKHR>(instFcts.vkGetPhysicalDeviceSurfaceFormatsKHR,
                                                                "surface formats", vkPhysicalDevice, vkSurface);
    const auto vkPresentModes = getVkList<VkPresentModeKHR>(instFcts.vkGetPhysicalDeviceSurfacePresentModesKHR,
                                                            "present modes", vkPhysicalDevice, vkSurface);

    logger.debug(fmt::format("Device for target supports {} formats and {} present modes", vkSurfaceFormats.size(),
                             vkPresentModes.size()));

    const auto imageFormat = chooseSurfaceFormat(logger, vkSurfaceFormats);
    const auto queueFamilyIndex = device.getCommandQueue()->getQueueFamilyIndex();

    return VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface,
        .minImageCount = determineImageCount(vkSurfaceCapabilities),
        .imageFormat = imageFormat.format,
        .imageColorSpace = imageFormat.colorSpace,
        .imageExtent = chooseExtent(logger, vkSurfaceCapabilities),
        .imageArrayLayers = 1U,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1U,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .preTransform = vkSurfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
}

auto createVkSwapchain(const IDevice& device, const VkSwapchainCreateInfoKHR& vkCreateInfo)
    -> VkUniquePtr<VkSwapchainKHR>
{
    const auto vkDevice = device.getVkDevice();
    const auto& fcts = device.getFcts();

    VkSwapchainKHR vkSwapchain{};
    throwIfVkFailed(fcts.vkCreateSwapchainKHR(vkDevice, &vkCreateInfo, nullptr, &vkSwapchain),
                    "Could not create swapchain for Presenter");

    return makeVkUniquePtr<VkSwapchainKHR>(vkDevice, vkSwapchain, fcts.vkDestroySwapchainKHR);
}

auto getSwapchainImages(const IDevice& device, VkSwapchainKHR vkSwapchain, const VkSwapchainCreateInfoKHR& vkCreateInfo)
    -> std::vector<std::shared_ptr<IImage>>
{
    const auto vkImages = getVkList<VkImage>(device.getFcts().vkGetSwapchainImagesKHR, "swapchain images",
                                             device.getVkDevice(), vkSwapchain);

    auto imageFactory = device.getImageFactory();

    std::vector<std::shared_ptr<IImage>> images;
    images.reserve(vkImages.size());
    std::ranges::transform(vkImages, std::back_inserter(images), [&, n = 0](const auto& vkImage) mutable {
        return imageFactory->createProxyImage(vkImage, ImageConfig{
                                                           .name = fmt::format("SwapchainImage{}", n++),
                                                           .vkExtent = vkCreateInfo.imageExtent,
                                                           .vkFormat = vkCreateInfo.imageFormat,
                                                           .vkUsage = vkCreateInfo.imageUsage,
                                                       });
    });
    return images;
}

auto makeVkPresentInfo(const VkSwapchainKHR* vkSwapchain, const uint32_t* imageIndex,
                       const VkSemaphore* vkWaitSemaphore) -> VkPresentInfoKHR
{
    return VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = vkWaitSemaphore,
        .swapchainCount = 1U,
        .pSwapchains = vkSwapchain,
        .pImageIndices = imageIndex,
        .pResults = nullptr,
    };
}

}  // namespace

Presenter::Presenter(std::shared_ptr<IDevice> device, VkSurfaceKHR vkSurface,
                     std::unique_ptr<IFramePipeline> framePipeline)
  : m_device(throwIfArgNull(std::move(device), "Presenter requires a device"))
  , m_vkSurface(throwIfArgNull(vkSurface, "Presenter requires a surface"))
  , m_framePipeline(throwIfArgNull(std::move(framePipeline), "Presenter requires a frame pipeline"))
  , m_logger(m_device->createLogger("Presenter"))
{
    reset();
}

Presenter::~Presenter()
{
    // Wait for the queue to be idle as there might still be some pending vkQueuePresentKHR commands.
    m_device->getCommandQueue()->waitIdle();
}

void Presenter::present()
{
    if (m_isOutOfDate)
    {
        m_logger->info("Swapchain currently out of date, a reset is needed");
        return;
    }

    const auto& fcts = m_device->getFcts();
    auto commandQueue = m_device->getCommandQueue();

    m_readyToWriteSemaphoreIndex = (m_readyToWriteSemaphoreIndex + 1U) % m_vkReadyToWriteSemaphores.size();
    auto vkReadyToWriteSemaphore = m_vkReadyToWriteSemaphores[m_readyToWriteSemaphoreIndex];
    uint32_t imageIndex{};
    {
        const auto vkResult = fcts.vkAcquireNextImageKHR(m_device->getVkDevice(), m_vkSwapchain.get(),
                                                         std::numeric_limits<uint64_t>::max(),
                                                         vkReadyToWriteSemaphore.get(), nullptr, &imageIndex);
        if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_isOutOfDate = true;
            m_logger->info("swapchain out of date: will need reset");
            return;  // return early, we cannot do anything until the presenter is reset
        }
        else if (vkResult == VK_SUBOPTIMAL_KHR)
        {
            // We successfully acquired a new image so do not reset just yet, wait until all semaphores, etc. are in a
            // complete state.
            m_logger->info("Suboptimal: a reset will be needed after this draw call");
        }
        else
        {
            throwIfVkFailed(vkResult, "Failed to acquire next swapchain image for presenter");
        }
    }
    m_commandIndex = (m_commandIndex + 1U) % m_commandFutures.size();
    auto vkReadyToPresentSemaphore = m_vkReadyToPresentSemaphores[m_commandIndex];
    if (m_commandFutures[m_commandIndex])
    {
        m_commandFutures[m_commandIndex]->waitForCompletion();
        m_commandFutures[m_commandIndex].reset();
    }
    {
        auto commandBuffer = commandQueue->startScopedCommand("present", CommandExecutionType::Async);
        commandBuffer->setVkWaitSemaphore(vkReadyToWriteSemaphore);
        commandBuffer->setVkSignalSemaphore(vkReadyToPresentSemaphore);

        m_framePipeline->prepareExecution(*commandBuffer, m_vkExtent, m_images[imageIndex]);
        {
            auto barrier = commandBuffer->startScopedBarrier("BeforePresentation");
            barrier->addImageBarrier(*m_images[imageIndex], ImageBarrierConfig{
                                                                .vkLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                            });
        }

        m_commandFutures[m_commandIndex] = commandBuffer->createFuture();
    }
    const auto vkSwapchain = m_vkSwapchain.get();
    const auto vkPresentWaitSemaphore = vkReadyToPresentSemaphore.get();
    const auto vkPresentInfo = makeVkPresentInfo(&vkSwapchain, &imageIndex, &vkPresentWaitSemaphore);
    const auto vkPresentResult = fcts.vkQueuePresentKHR(commandQueue->getVkQueue(), &vkPresentInfo);
    if (vkPresentResult == VK_ERROR_OUT_OF_DATE_KHR || vkPresentResult == VK_SUBOPTIMAL_KHR)
    {
        m_isOutOfDate = true;
        m_logger->info("Invalid state detected: a reset will be needed");
    }
    else
    {
        throwIfVkFailed(vkPresentResult, "Failed to present image");
    }
}

void Presenter::reset()
{
    // Wait for any future we have left to make sure our swapchain images have all been processed:
    std::ranges::for_each(m_commandFutures, [](auto& future) {
        if (future)
        {
            future->waitForCompletion();
        }
    });
    m_commandFutures.clear();

    // Wait for the queue to be idle as there might still be images being presented via vkQueuePresentKHR and we
    // cannot have a fence for these calls:
    m_device->getCommandQueue()->waitIdle();

    m_vkSwapchain.reset();

    const auto vkCreateInfo = createVkSwapchainCreateInfo(*m_logger, *m_device, m_vkSurface);
    m_vkSwapchain = createVkSwapchain(*m_device, vkCreateInfo);
    m_vkExtent = vkCreateInfo.imageExtent;
    m_images = getSwapchainImages(*m_device, m_vkSwapchain.get(), vkCreateInfo);

    auto createSemaphore = [&](auto& vkSemaphore) { vkSemaphore = createVkSemaphore(*m_device); };

    // We add 1 one more ready to write semaphore because we could start acquiring the next image while all images
    // are currently in flight.
    m_vkReadyToWriteSemaphores.resize(m_images.size() + 1U);
    std::ranges::for_each(m_vkReadyToWriteSemaphores, createSemaphore);
    m_readyToWriteSemaphoreIndex = {};

    m_vkReadyToPresentSemaphores.resize(m_images.size());
    std::ranges::for_each(m_vkReadyToPresentSemaphores, createSemaphore);
    m_commandFutures.resize(m_images.size());
    m_commandIndex = {};

    m_framePipeline->resize(m_vkExtent, static_cast<uint32_t>(m_images.size()));

    m_isOutOfDate = false;
    m_logger->debug(fmt::format("Successfully initialized swapchain with {} images", m_images.size()));
}
