#pragma once

#include "anari_device.h"

#include <im3e/api/gui.h>
#include <im3e/scene/camera.h>
#include <im3e/utils/loggers.h>

#include <numbers>

namespace im3e {

class AnariMapCamera : public IGuiEventListener
{
public:
    AnariMapCamera(std::shared_ptr<AnariDevice> pAnDevice);
    AnariMapCamera(const AnariMapCamera&) = delete;
    AnariMapCamera(AnariMapCamera&&) = delete;
    AnariMapCamera& operator=(const AnariMapCamera&) = delete;
    AnariMapCamera& operator=(AnariMapCamera&&) = delete;

    void commitChanges();

    auto createProperties() -> std::shared_ptr<IPropertyGroup>;

    void onMouseMove(const MouseMoveEvent& rEvent) override;
    void onMouseWheel(float scrollSteps) override;

    void setAspectRatio(float ratio);

    auto getHandle() const -> ANARICamera { return m_pAnCamera.get(); }
    auto getViewFrustum() const -> const ViewFrustum& { return m_camera.getViewFrustum(); }

private:
    void _update();

    std::shared_ptr<AnariDevice> m_pAnDevice;
    std::unique_ptr<ILogger> m_pLogger;
    std::shared_ptr<anari::api::Camera> m_pAnCamera;

    bool m_needsCommit{true};

    glm::vec3 m_targetPoint{3700.0F, 0.0F, 4000.0F};
    float m_distanceToTarget{5000.0F};
    float m_angleX{};
    float m_angleY{};

    Camera m_camera;
};

}  // namespace im3e