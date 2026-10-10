#include <im3e/guis/guis.h>

#include <im3e/test_utils/integration_test.h>

using namespace im3e;

struct GlfwWindowApplicationIntegrationTest : public IntegrationTest
{
    auto createApplication()
    {
        return createGlfwWindowApplication(getLogger(), WindowApplicationConfig{
                                                            .name = getName(),
                                                            .isDebugEnabled = true,
                                                        });
    }
};

TEST_F(GlfwWindowApplicationIntegrationTest, constructor)
{
    auto pApp = createApplication();
    ASSERT_THAT(pApp, NotNull());
    ASSERT_THAT(pApp->getDevice(), NotNull());
}

TEST_F(GlfwWindowApplicationIntegrationTest, runWithoutWindow)
{
    auto pApp = createApplication();
    EXPECT_NO_THROW(pApp->run());
}

TEST_F(GlfwWindowApplicationIntegrationTest, createWindowWithoutRun)
{
    auto pApp = createApplication();
    auto pWorkspace = createImguiWorkspace("test_workspace");
    pApp->createWindow(WindowConfig{}, pWorkspace);
}

TEST_F(GlfwWindowApplicationIntegrationTest, createWindow)
{
    auto pApp = createApplication();
    auto pWorkspace = createImguiWorkspace("workspace");
    pApp->createWindow(WindowConfig{}, pWorkspace);

    pApp->run([&, n = 0]() mutable {
        if (n == 3U)
        {
            pApp->stop();
        }
        n++;
    });
}

TEST_F(GlfwWindowApplicationIntegrationTest, createMultipleWindows)
{
    auto pApp = createApplication();

    auto pWorkspace1 = createImguiWorkspace("workspace1");
    pApp->createWindow(WindowConfig{}, pWorkspace1);

    auto pWorkspace2 = createImguiWorkspace("workspace2");
    pApp->createWindow(WindowConfig{}, pWorkspace2);

    pApp->run([&, n = 0]() mutable {
        if (n == 10U)
        {
            pApp->stop();
        }
        n++;
    });
}