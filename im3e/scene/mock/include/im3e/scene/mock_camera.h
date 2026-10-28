#pragma once

#include <im3e/scene/camera.h>
#include <im3e/test_utils/test_utils.h>

#include <glm/glm.hpp>

namespace im3e {

class MockCamera : public ICamera
{
public:
    MockCamera();
    ~MockCamera() override;

    MOCK_METHOD(Ray, createViewRayFromNdc, (const glm::vec2&), (const, override));

    MOCK_METHOD(std::shared_ptr<IPropertyGroup>, createProperties, (), (override));

    MOCK_METHOD(PerspectiveProjection&, getProjection, (), (override));
    MOCK_METHOD(const PerspectiveProjection&, getProjection, (), (const, override));
    MOCK_METHOD(ViewTransform&, getView, (), (override));
    MOCK_METHOD(const ViewTransform&, getView, (), (const, override));
    MOCK_METHOD(const ViewFrustum&, getViewFrustum, (), (const, override));

    auto getMockOnChanged() -> MockFunction<void()>& { return m_mockOnChanged; }
    auto getMockProjection() -> PerspectiveProjection& { return m_projection; }
    auto getMockProjection() const -> const PerspectiveProjection& { return m_projection; }
    auto getMockView() -> ViewTransform& { return m_view; }
    auto getMockView() const -> const ViewTransform& { return m_view; }

private:
    NiceMock<MockFunction<void()>> m_mockOnChanged;

    PerspectiveProjection m_projection;
    ViewTransform m_view;
    ViewFrustum m_viewFrustum;
};

}  // namespace im3e