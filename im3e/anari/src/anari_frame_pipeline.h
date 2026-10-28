#pragma once

#include "anari_device.h"
#include "anari_map_camera.h"
#include "anari_renderer.h"
#include "anari_world.h"

#include <im3e/api/device.h>
#include <im3e/api/frame_pipeline.h>
#include <im3e/api/image.h>
#include <im3e/utils/core/types.h>

#include <anari/anari.h>

#include <memory>

namespace im3e {

class AnariFramePipeline : public IAnariFramePipeline
{
public:
    AnariFramePipeline(std::shared_ptr<IDevice> pDevice, std::shared_ptr<AnariDevice> pAnDevice);

    void prepareExecution(const ICommandBuffer& rCommandBuffer, const VkExtent2D& rVkViewportSize,
                          std::shared_ptr<IImage> pOutputImage) override;

    void resize(const VkExtent2D& rWindowSize, uint32_t frameInFlightCount) override;

    auto createCameraProperties() -> std::shared_ptr<IPropertyGroup> override;
    auto createRendererProperties() -> std::shared_ptr<IPropertyGroup> override;

    auto getCameraListener() -> std::shared_ptr<IGuiEventListener> override { return m_pCamera; }
    auto getWorld() -> std::shared_ptr<IAnariWorld> override { return m_pAnWorld; }
    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_pDevice; }

private:
    std::shared_ptr<IDevice> m_pDevice;
    std::shared_ptr<AnariDevice> m_pAnDevice;
    std::unique_ptr<ILogger> m_pLogger;
    std::unique_ptr<AnariRenderer> m_pAnRenderer;
    std::shared_ptr<AnariWorld> m_pAnWorld;

    std::shared_ptr<AnariMapCamera> m_pCamera;
    UniquePtrWithDeleter<anari::api::Frame> m_pAnFrame;
    std::unique_ptr<IHostVisibleImage> m_pImage;

    VkExtent2D m_currentViewportSize{};
    bool m_renderingFrame = false;
};

}  // namespace im3e