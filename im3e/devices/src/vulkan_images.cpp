#include "vulkan_images.h"

#include "vulkan_device.h"

#include <im3e/utils/core/throw_utils.h>

#include <CImg.h>
#include <fmt/format.h>

using namespace im3e;

namespace {

void setVkObjectDebugName(const IDevice& rDevice, VkObjectType vkObjectType, void* pVkHandle, std::string_view name)
{
    auto& rFcts = rDevice.getFcts();
    if (!rFcts.vkSetDebugUtilsObjectNameEXT)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT vkDebugInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = vkObjectType,
        .objectHandle = reinterpret_cast<uint64_t>(pVkHandle),
        .pObjectName = name.data(),
    };
    throwIfVkFailed(rFcts.vkSetDebugUtilsObjectNameEXT(rDevice.getVkDevice(), &vkDebugInfo),
                    fmt::format("Failed to set debug name to image \"{}\"", name));
}

struct VulkanImageBuffer
{
    VulkanImageBuffer(std::shared_ptr<const IDevice> pDevice, VkImage vkImage, ImageConfig config)
      : m_pDevice(throwIfArgNull(std::move(pDevice), "Vulkan image buffer requires a device"))
      , m_config(std::move(config))
      , m_vkImage(throwIfArgNull(vkImage, "Cannot create Vulkan proxy image without an image"))
    {
        setVkObjectDebugName(*m_pDevice, VK_OBJECT_TYPE_IMAGE, m_vkImage, fmt::format("Im3eImage.{}", m_config.name));
    }

    VulkanImageBuffer(std::shared_ptr<const IDevice> pDevice, std::shared_ptr<IVulkanMemoryAllocator> pMemoryAllocator,
                      ImageConfig config, VkImageTiling vkTiling, VmaMemoryUsage vmaMemoryUsage)
      : m_pDevice(throwIfArgNull(std::move(pDevice), "Vulkan image buffer requires a device"))
      , m_pMemoryAllocator(
            throwIfArgNull(std::move(pMemoryAllocator), "Cannot create Vulkan image without a memory allocator"))
      , m_config(std::move(config))
    {
        VkImageCreateInfo vkCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = m_config.vkCreateFlags,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = m_config.vkFormat,
            .extent{
                .width = m_config.vkExtent.width,
                .height = m_config.vkExtent.height,
                .depth = 1U,
            },
            .mipLevels = 1U,
            .arrayLayers = 1U,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = vkTiling,
            .usage = m_config.vkUsage,
        };

        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        VkMemoryPropertyFlags vkRequiredFlags{};
        if (vmaMemoryUsage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
        {
            vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }

        VmaAllocationCreateInfo vmaCreateInfo{
            .flags = vmaFlags,
            .usage = vmaMemoryUsage,
            .requiredFlags = vkRequiredFlags,
            .pUserData = const_cast<char*>(m_config.name.c_str()),
        };
        throwIfVkFailed(m_pMemoryAllocator->createImage(&vkCreateInfo, &vmaCreateInfo, &m_vkImage, &m_vmaAllocation,
                                                        &m_vmaAllocationInfo),
                        fmt::format("Failed to create image \"{}\" with VMA", m_config.name));

        setVkObjectDebugName(*m_pDevice, VK_OBJECT_TYPE_IMAGE, m_vkImage, fmt::format("Im3eImage.{}", m_config.name));
    }

    ~VulkanImageBuffer()
    {
        if (m_pMemoryAllocator)
        {
            m_pMemoryAllocator->destroyImage(m_vkImage, m_vmaAllocation);
        }
    }

    auto getVkSubresourceLayers() const
    {
        return VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1U,
        };
    }

    std::shared_ptr<const IDevice> m_pDevice;
    std::shared_ptr<IVulkanMemoryAllocator> m_pMemoryAllocator;
    const ImageConfig m_config;

    VkImage m_vkImage{};
    VmaAllocation m_vmaAllocation{};
    VmaAllocationInfo m_vmaAllocationInfo{};
};

auto getAspectMaskFromImageUsage(VkImageUsageFlags vkImageUsage) -> VkImageAspectFlags
{
    if (vkFlagsContain(vkImageUsage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
    {
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

class VulkanImageView : public IImageView
{
public:
    VulkanImageView(std::shared_ptr<const VulkanImageBuffer> pImageBuffer)
      : m_pImageBuffer(throwIfArgNull(std::move(pImageBuffer), "Cannot create a Vulkan image view without an image"))
    {
        VkImageViewCreateInfo vkCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_pImageBuffer->m_vkImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_pImageBuffer->m_config.vkFormat,
            .subresourceRange{
                .aspectMask = getAspectMaskFromImageUsage(m_pImageBuffer->m_config.vkUsage),
                .levelCount = 1U,
                .layerCount = 1U,
            },
        };

        const auto vkDevice = m_pImageBuffer->m_pDevice->getVkDevice();
        const auto& rFcts = m_pImageBuffer->m_pDevice->getFcts();

        VkImageView vkImageView{};
        throwIfVkFailed(rFcts.vkCreateImageView(vkDevice, &vkCreateInfo, nullptr, &vkImageView),
                        "Could not create image buffer view");
        m_pVkImageView = makeVkUniquePtr<VkImageView>(vkDevice, vkImageView, rFcts.vkDestroyImageView);

        setVkObjectDebugName(*m_pImageBuffer->m_pDevice, VK_OBJECT_TYPE_IMAGE_VIEW, m_pVkImageView.get(),
                             fmt::format("Im3eImageView.{}", m_pImageBuffer->m_config.name));
    }

    auto getVkImageView() const -> VkImageView override { return m_pVkImageView.get(); }
    auto getVkImage() const -> VkImage override { return m_pImageBuffer->m_vkImage; }

private:
    std::shared_ptr<const VulkanImageBuffer> m_pImageBuffer;
    VkUniquePtr<VkImageView> m_pVkImageView;
};

class VulkanImageMetadata : public IImageMetadata
{
public:
    void setLayout(VkImageLayout vkLayout) override { m_vkLayout = vkLayout; }
    void setLastStageMask(VkPipelineStageFlags2 vkLastStageMask) override { m_vkLastStageMask = vkLastStageMask; }
    void setLastAccessMask(VkAccessFlags2 vkLastAccessMask) override { m_vkLastAccessMask = vkLastAccessMask; }

    auto getLayout() const -> VkImageLayout override { return m_vkLayout; }
    auto getQueueFamilyIndex() const -> uint32_t override { return VK_QUEUE_FAMILY_IGNORED; }
    auto getLastStageMask() const -> VkPipelineStageFlags2 override { return m_vkLastStageMask; }
    auto getLastAccessMask() const -> VkAccessFlags2 override { return m_vkLastAccessMask; }

private:
    VkImageLayout m_vkLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 m_vkLastStageMask = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 m_vkLastAccessMask = VK_ACCESS_2_NONE;
};

class VulkanImage : public IImage
{
public:
    VulkanImage(std::shared_ptr<const IDevice> pDevice, std::shared_ptr<IVulkanMemoryAllocator> pMemoryAllocator,
                ImageConfig config)
      : m_pImageBuffer(std::make_shared<VulkanImageBuffer>(std::move(pDevice), std::move(pMemoryAllocator),
                                                           std::move(config), VK_IMAGE_TILING_OPTIMAL,
                                                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE))
      , m_pMetadata(std::make_shared<VulkanImageMetadata>())
    {
    }

    auto createView() const -> std::unique_ptr<IImageView> override
    {
        return std::make_unique<VulkanImageView>(m_pImageBuffer);
    }

    auto getVkImage() const -> VkImage override { return m_pImageBuffer->m_vkImage; }
    auto getVkExtent() const -> VkExtent2D override { return m_pImageBuffer->m_config.vkExtent; }
    auto getVkFormat() const -> VkFormat override { return m_pImageBuffer->m_config.vkFormat; }
    auto getVkSubresourceLayers() const -> VkImageSubresourceLayers override
    {
        return m_pImageBuffer->getVkSubresourceLayers();
    }
    auto getMetadata() -> std::shared_ptr<IImageMetadata> override { return m_pMetadata; }
    auto getMetadata() const -> std::shared_ptr<const IImageMetadata> override { return m_pMetadata; }

private:
    std::shared_ptr<VulkanImageBuffer> m_pImageBuffer;
    std::shared_ptr<IImageMetadata> m_pMetadata;
};

auto mapHostVisibleMemory(std::shared_ptr<IVulkanMemoryAllocator> pAllocator, VmaAllocation vmaAllocation)
{
    void* pData{};
    throwIfVkFailed(pAllocator->mapMemory(vmaAllocation, &pData), "Failed to map host-visible image memory");

    return UniquePtrWithDeleter<uint8_t>(reinterpret_cast<uint8_t*>(pData), [pAllocator, vmaAllocation](uint8_t*) {
        pAllocator->unmapMemory(vmaAllocation);
    });
}

auto queryImageRowPitch(const IDevice& rDevice, VkImage vkImage)
{
    const auto vkDevice = rDevice.getVkDevice();
    const auto& rFcts = rDevice.getFcts();

    VkImageSubresource vkSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

    VkSubresourceLayout vkResult{};
    rFcts.vkGetImageSubresourceLayout(vkDevice, vkImage, &vkSubresource, &vkResult);

    return vkResult.rowPitch;
}

class VulkanHostVisibleImageMapping : public IHostVisibleImage::IMapping
{
public:
    VulkanHostVisibleImageMapping(std::shared_ptr<VulkanImageBuffer> pImageBuffer)
      : m_pImageBuffer(throwIfArgNull(std::move(pImageBuffer), "Host-visible image mapping requires a buffer"))
      , m_pData(mapHostVisibleMemory(m_pImageBuffer->m_pMemoryAllocator, m_pImageBuffer->m_vmaAllocation))
      , m_formatProperties(getFormatProperties(m_pImageBuffer->m_config.vkFormat))
      , m_vkExtent(m_pImageBuffer->m_config.vkExtent)
      , m_rowPitch(queryImageRowPitch(*m_pImageBuffer->m_pDevice, m_pImageBuffer->m_vkImage))
    {
    }

    void save(const std::filesystem::path& rFileName) const override
    {
        // Disable log messages from CImg:
        cimg_library::cimg::exception_mode(0U);

        // CImg memory is not interleaved i.e. each channel is stored separately one after the other.
        // To go around that, we allocate the image with swapped components to which we can save our data and permute
        // axes afterwards to make it readable from CImg:
        cimg_library::CImg<uint8_t> image(m_formatProperties.componentCount, m_vkExtent.width, m_vkExtent.height, 1U,
                                          false);

        // There is no concept of memory padding in CImg so we have to copy per row in case our current image has any
        // padding at the end of each row:
        auto* pSrcData = m_pData.get();
        auto* pDstData = image.data();
        const auto dstRowSize = m_vkExtent.width * m_formatProperties.sizeInBytes;
        for (auto row = 0U; row < m_vkExtent.height; row++)
        {
            std::copy(pSrcData, pSrcData + dstRowSize, pDstData);
            pSrcData += m_rowPitch;
            pDstData += dstRowSize;
        }

        image.permute_axes("yzcx");
        image.save(rFileName.string().c_str());
    }

    auto getData() -> uint8_t* override { return m_pData.get(); }
    auto getConstData() const -> const uint8_t* override { return m_pData.get(); }
    auto getSizeInBytes() const -> VkDeviceSize override { return m_pImageBuffer->m_vmaAllocationInfo.size; }
    auto getRowPitch() const -> VkDeviceSize override { return m_rowPitch; }
    auto getPixel(uint32_t x, uint32_t y) const -> const uint8_t* override
    {
        return m_pData.get() + y * m_rowPitch + x * m_formatProperties.sizeInBytes;
    }

private:
    std::shared_ptr<VulkanImageBuffer> m_pImageBuffer;
    UniquePtrWithDeleter<uint8_t> m_pData;
    const FormatProperties m_formatProperties{};
    const VkExtent2D m_vkExtent{};
    const VkDeviceSize m_rowPitch{};
};

class VulkanHostVisibleImage : public IHostVisibleImage
{
public:
    VulkanHostVisibleImage(std::shared_ptr<const IDevice> pDevice,
                           std::shared_ptr<IVulkanMemoryAllocator> pMemoryAllocator, ImageConfig config)
      : m_pImageBuffer(std::make_shared<VulkanImageBuffer>(std::move(pDevice), std::move(pMemoryAllocator),
                                                           std::move(config), VK_IMAGE_TILING_LINEAR,
                                                           VMA_MEMORY_USAGE_AUTO_PREFER_HOST))
      , m_pMetadata(std::make_shared<VulkanImageMetadata>())
    {
    }

    auto createView() const -> std::unique_ptr<IImageView> override
    {
        return std::make_unique<VulkanImageView>(m_pImageBuffer);
    }

    auto map() -> std::unique_ptr<IMapping> override
    {
        return std::make_unique<VulkanHostVisibleImageMapping>(m_pImageBuffer);
    }
    auto mapReadOnly() const -> std::unique_ptr<const IMapping> override
    {
        return std::make_unique<VulkanHostVisibleImageMapping>(m_pImageBuffer);
    }

    auto getVkImage() const -> VkImage override { return m_pImageBuffer->m_vkImage; }
    auto getVkExtent() const -> VkExtent2D override { return m_pImageBuffer->m_config.vkExtent; }
    auto getVkFormat() const -> VkFormat override { return m_pImageBuffer->m_config.vkFormat; }
    auto getVkSubresourceLayers() const -> VkImageSubresourceLayers override
    {
        return m_pImageBuffer->getVkSubresourceLayers();
    }
    auto getMetadata() -> std::shared_ptr<IImageMetadata> override { return m_pMetadata; }
    auto getMetadata() const -> std::shared_ptr<const IImageMetadata> override { return m_pMetadata; }

private:
    std::shared_ptr<VulkanImageBuffer> m_pImageBuffer;
    std::shared_ptr<VulkanImageMetadata> m_pMetadata;
};

class VulkanProxyImage : public IImage
{
public:
    VulkanProxyImage(std::shared_ptr<const IDevice> pDevice, VkImage vkImage, ImageConfig config)
      : m_pImageBuffer(std::make_shared<VulkanImageBuffer>(std::move(pDevice), vkImage, std::move(config)))
      , m_pMetadata(std::make_shared<VulkanImageMetadata>())
    {
    }

    auto createView() const -> std::unique_ptr<IImageView> override
    {
        return std::make_unique<VulkanImageView>(m_pImageBuffer);
    }

    auto getVkImage() const -> VkImage override { return m_pImageBuffer->m_vkImage; }
    auto getVkExtent() const -> VkExtent2D override { return m_pImageBuffer->m_config.vkExtent; }
    auto getVkFormat() const -> VkFormat override { return m_pImageBuffer->m_config.vkFormat; }
    auto getVkSubresourceLayers() const -> VkImageSubresourceLayers override
    {
        return m_pImageBuffer->getVkSubresourceLayers();
    }
    auto getMetadata() -> std::shared_ptr<IImageMetadata> override { return m_pMetadata; }
    auto getMetadata() const -> std::shared_ptr<const IImageMetadata> override { return m_pMetadata; }

private:
    std::shared_ptr<VulkanImageBuffer> m_pImageBuffer;
    std::shared_ptr<IImageMetadata> m_pMetadata;
};

class VulkanImageFactory : public IImageFactory
{
public:
    VulkanImageFactory(std::weak_ptr<const IDevice> pDevice, std::shared_ptr<IVulkanMemoryAllocator> pMemoryAllocator)
      : m_pDevice(std::move(pDevice))
      , m_pMemoryAllocator(
            throwIfArgNull(std::move(pMemoryAllocator), "Cannot create Vulkan image factory without memory allocator"))
    {
    }

    auto createImage(ImageConfig config) const -> std::unique_ptr<IImage> override
    {
        if (auto pDevice = m_pDevice.lock())
        {
            return std::make_unique<VulkanImage>(std::move(pDevice), m_pMemoryAllocator, std::move(config));
        }
        return nullptr;
    }

    auto createHostVisibleImage(ImageConfig config) const -> std::unique_ptr<IHostVisibleImage> override
    {
        if (auto pDevice = m_pDevice.lock())
        {
            return std::make_unique<VulkanHostVisibleImage>(std::move(pDevice), m_pMemoryAllocator, std::move(config));
        }
        return nullptr;
    }

    auto createProxyImage(VkImage vkImage, ImageConfig config) const -> std::unique_ptr<IImage> override
    {
        if (auto pDevice = m_pDevice.lock())
        {
            return std::make_unique<VulkanProxyImage>(std::move(pDevice), vkImage, std::move(config));
        }
        return nullptr;
    }

private:
    std::weak_ptr<const IDevice> m_pDevice;
    std::shared_ptr<IVulkanMemoryAllocator> m_pMemoryAllocator;
};

}  // namespace

auto im3e::createVulkanImageFactory(std::weak_ptr<const IDevice> pDevice,
                                    std::shared_ptr<IVulkanMemoryAllocator> pMemoryAllocator)
    -> std::unique_ptr<IImageFactory>
{
    return std::make_unique<VulkanImageFactory>(std::move(pDevice), std::move(pMemoryAllocator));
}