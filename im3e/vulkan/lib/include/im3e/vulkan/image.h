#pragma once

#include <im3e/utils/vk_utils.h>

#include <filesystem>

namespace im3e {

struct ImageConfig
{
    std::string name;
    VkExtent2D vkExtent{};
    VkFormat vkFormat = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags vkUsage{};
    VkImageCreateFlags vkCreateFlags{};
};

class IImageView
{
public:
    virtual ~IImageView() = default;

    virtual auto getVkImageView() const -> VkImageView = 0;
    virtual auto getVkImage() const -> VkImage = 0;
};

class IImageMetadata
{
public:
    virtual ~IImageMetadata() = default;

    virtual void setLayout(VkImageLayout vkLayout) = 0;
    virtual void setLastStageMask(VkPipelineStageFlags2 vkStageMask) = 0;
    virtual void setLastAccessMask(VkAccessFlags2 vkAccessMask) = 0;

    virtual auto getLayout() const -> VkImageLayout = 0;
    virtual auto getQueueFamilyIndex() const -> uint32_t = 0;
    virtual auto getLastStageMask() const -> VkPipelineStageFlags2 = 0;
    virtual auto getLastAccessMask() const -> VkAccessFlags2 = 0;
};

class IImage
{
public:
    virtual ~IImage() = default;

    virtual auto createView() const -> std::unique_ptr<IImageView> = 0;

    virtual auto getVkImage() const -> VkImage = 0;
    virtual auto getVkExtent() const -> VkExtent2D = 0;
    virtual auto getVkFormat() const -> VkFormat = 0;
    virtual auto getVkSubresourceLayers() const -> VkImageSubresourceLayers = 0;
    virtual auto getMetadata() -> std::shared_ptr<IImageMetadata> = 0;
    virtual auto getMetadata() const -> std::shared_ptr<const IImageMetadata> = 0;
};

class IHostVisibleImage : public IImage
{
public:
    virtual ~IHostVisibleImage() = default;

    class IMapping
    {
    public:
        virtual ~IMapping() = default;

        virtual void save(const std::filesystem::path& rFileName) const = 0;

        virtual auto getData() -> uint8_t* = 0;
        virtual auto getConstData() const -> const uint8_t* = 0;
        virtual auto getSizeInBytes() const -> VkDeviceSize = 0;

        /// @return Number of bytes between each row in the mapped buffer.
        virtual auto getRowPitch() const -> VkDeviceSize = 0;

        virtual auto getPixel(uint32_t x, uint32_t y) const -> const uint8_t* = 0;
    };

    /// @brief Map image data to access it from the CPU.
    /// @details Mapped data is guaranteed to remain valid until it is no longer referenced by any mapping, even if the
    /// original image is destroyed.
    virtual auto map() -> std::unique_ptr<IMapping> = 0;
    virtual auto mapReadOnly() const -> std::unique_ptr<const IMapping> = 0;
};

class IImageFactory
{
public:
    virtual ~IImageFactory() = default;

    virtual auto createImage(ImageConfig config) const -> std::unique_ptr<IImage> = 0;
    virtual auto createHostVisibleImage(ImageConfig config) const -> std::unique_ptr<IHostVisibleImage> = 0;
    virtual auto createProxyImage(VkImage vkImage, ImageConfig config) const -> std::unique_ptr<IImage> = 0;
};

}  // namespace im3e