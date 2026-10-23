#pragma once

#include "anari_device.h"

#include <im3e/api/gui.h>
#include <im3e/utils/camera_transforms.h>
#include <im3e/utils/loggers.h>
#include <im3e/utils/view_frustum.h>

#include <numbers>

namespace im3e {

class AnariMapCamera : public IGuiEventListener
{
public:
    AnariMapCamera(std::shared_ptr<AnariDevice> pAnDevice);

    void commitChanges();

    auto createProperties() -> std::shared_ptr<IPropertyGroup>;

    void onMouseMove(const glm::vec2& rClipOffset, MouseButtons mouseButtons, KeyModifiers keyModifiers) override;
    void onMouseWheel(float scrollSteps) override;

    void setAspectRatio(float ratio);

    auto getHandle() const -> ANARICamera { return m_pAnCamera.get(); }
    auto getViewFrustum() const -> const ViewFrustum& { return m_viewFrustum; }

private:
    void _update();
    auto _createViewFrustum() const -> ViewFrustum;

    std::shared_ptr<AnariDevice> m_pAnDevice;
    std::unique_ptr<ILogger> m_pLogger;
    std::shared_ptr<anari::api::Camera> m_pAnCamera;

    bool m_needsCommit{true};
    std::shared_ptr<std::function<void()>> m_pOnTransformChanged;

    glm::vec3 m_targetPoint{};
    float m_distanceToTarget{1400.0F};
    float m_angleX{};
    float m_angleY{};

    PerspectiveProjection m_perspective;
    ViewTransform m_view;

    ViewFrustum m_viewFrustum;
};

}  // namespace im3e