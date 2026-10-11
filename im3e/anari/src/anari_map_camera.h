#pragma once

#include "anari_device.h"

#include <im3e/api/gui.h>
#include <im3e/utils/loggers.h>
#include <im3e/utils/view_frustum.h>

#include <numbers>

namespace im3e {

class AnariMapCamera : public IGuiEventListener
{
public:
    AnariMapCamera(std::shared_ptr<AnariDevice> pAnDevice);

    void commitChanges();

    void onMouseMove(const glm::vec2& rClipOffset, const std::array<bool, 3U>& rMouseButtonsDown) override;
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

    struct PerspectiveState
    {
        float fovY{std::numbers::pi_v<float> / 3.0F};
        float aspectRatio{1.0F};
        float near{0.1F};
        float far{10'000.0F};

        void setCameraParameters(ANARIDevice anDevice, ANARICamera anCamera) const;

        auto generateMatrix() const -> glm::mat4;
    };
    PerspectiveState m_perspective;

    struct ViewState
    {
        glm::vec3 targetPoint{};
        float distanceToTarget{1400.0F};
        float angleX{};
        float angleY{};

        void update();
        void setCameraParameters(ANARIDevice anDevice, ANARICamera anCamera) const;

        auto generateMatrix() const -> glm::mat4;

        auto getPosition() const -> const glm::vec3& { return m_position; }
        auto getDirection() const -> const glm::vec3& { return m_direction; }
        auto getUp() const -> const glm::vec3& { return m_up; }
        auto getRight() const -> const glm::vec3& { return m_right; }

    private:
        glm::vec3 m_position;
        glm::vec3 m_direction;
        glm::vec3 m_up;
        glm::vec3 m_right;
    };
    ViewState m_view;

    ViewFrustum m_viewFrustum;
};

}  // namespace im3e