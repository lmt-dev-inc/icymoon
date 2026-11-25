#include <im3e/guis/guis.h>
#include <im3e/render/vulkan/vulkan_render_engine.h>
#include <im3e/utils/loggers.h>

using namespace im3e;

namespace {

constexpr bool DebugEnabled = true;

}  // namespace

int main()
{
    auto logger = createTerminalLogger();
    logger->setLevelFilter(LogLevel::Verbose);
    logger->info("IcyMoon Viewer");

    auto app = createGlfwWindowApplication(*logger, WindowApplicationConfig{
                                                        .name = "Viewer",
                                                        .isDebugEnabled = DebugEnabled,
                                                    });

    auto device = app->getDevice();
    auto renderEngine = createVulkanRenderEngine(device);
    auto renderPipeline = renderEngine->createFramePipeline();

    // Main Window
    {
        auto guiWorkspace = createImguiWorkspace("Workspace");
        guiWorkspace->addPanel(IGuiWorkspace::Location::Center,
                               createImguiRenderPanel("Renderer", std::move(renderPipeline)));
        guiWorkspace->addPanel(IGuiWorkspace::Location::Bottom,
                               createImguiStatsPanel("Stats", device->getStatsProvider()));
        app->createWindow(WindowConfig{}, guiWorkspace);
    }

    app->run();
    return 0;
}