#include <im3e/scene/mock_camera.h>

using namespace im3e;

MockCamera::MockCamera()
  : m_projection(m_mockOnChanged.AsStdFunction())
  , m_view(m_mockOnChanged.AsStdFunction())
  , m_viewFrustum(createViewFrustum(m_projection, m_view))
{
    ON_CALL(*this, getProjection()).WillByDefault(ReturnRef(m_projection));
    ON_CALL(Const(*this), getProjection()).WillByDefault(ReturnRef(m_projection));
    ON_CALL(*this, getView()).WillByDefault(ReturnRef(m_view));
    ON_CALL(Const(*this), getView()).WillByDefault(ReturnRef(m_view));
    ON_CALL(*this, getViewFrustum()).WillByDefault(ReturnRef(m_viewFrustum));
}

MockCamera::~MockCamera() = default;