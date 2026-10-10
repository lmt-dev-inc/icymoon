#include "src/glfw_window.h"

#include "src/glfw_window_application.h"
#include "src/imgui_workspace.h"

#include "clear_color_test_pipeline.h"

#include <im3e/devices/devices.h>
#include <im3e/test_utils/integration_test.h>
#include <im3e/test_utils/test_utils.h>

using namespace im3e;

struct GlfwWindowIntegrationTest : public IntegrationTest
{
    GlfwWindowIntegrationTest()
      : m_app(getLogger(), WindowApplicationConfig{.name = "TestApp", .isDebugEnabled = true})
    {
    }

    auto createWindow(std::shared_ptr<ImguiWorkspace> pWorkspace = std::make_shared<ImguiWorkspace>("workspace"))
    {
        return std::make_shared<GlfwWindow>(m_app.getDevice(),
                                            GlfwWindow::Config{
                                                .maximized = false,
                                            },
                                            pWorkspace);
    }

    auto refreshWindow(GlfwWindow& rWindow, uint32_t refreshCount = 1U)
    {
        for (uint32_t i = 0U; i < refreshCount; i++)
        {
            glfwPollEvents();
            rWindow.draw();
        }
    }

    GlfwWindowApplication m_app;
};

TEST_F(GlfwWindowIntegrationTest, constructor)
{
    auto pWindow = createWindow();
    ASSERT_THAT(pWindow, NotNull());
}

TEST_F(GlfwWindowIntegrationTest, resize)
{
    auto pWindow = createWindow();
    auto pGlfwWindow = pWindow->getHandle();

    refreshWindow(*pWindow);
    glfwSetWindowSize(pGlfwWindow, 800U, 600U);
    refreshWindow(*pWindow);
}

TEST_F(GlfwWindowIntegrationTest, resizeWithRenderPanel)
{
    auto pRenderPanel = createImguiRenderPanel("render", std::make_unique<ClearColorTestPipeline>(m_app.getDevice()));
    auto pWorkspace = std::make_shared<ImguiWorkspace>("workspace");
    pWorkspace->addPanel(IGuiWorkspace::Location::Center, pRenderPanel);
    auto pWindow = createWindow(pWorkspace);
    auto pGlfwWindow = pWindow->getHandle();

    refreshWindow(*pWindow, 10U);
    glfwSetWindowSize(pGlfwWindow, 800U, 600U);
    refreshWindow(*pWindow, 2U);
}

TEST_F(GlfwWindowIntegrationTest, setImguiDemoVisible)
{
    auto pWorkspace = std::make_shared<ImguiWorkspace>("workspace");
    auto pWindow = createWindow(pWorkspace);

    for (auto i = 0U; i < 10U; i++)
    {
        if (i == 2U)
        {
            pWorkspace->setImguiDemoVisible(true);
        }

        glfwPollEvents();
        pWindow->draw();
    }
}