#include <im3e/guis/guis.h>
#include <im3e/utils/loggers.h>

using namespace im3e;

int main([[maybe_unused]] int argc, char* argv[])
{
    auto pLogger = createTerminalLogger();
    auto pApp = createGlfwWindowApplication(*pLogger, WindowApplicationConfig{
                                                          .name = argv[0],
                                                          .isDebugEnabled = true,
                                                      });

    auto pWorkspace1 = createImguiWorkspace("Workspace 1");
    pApp->createWindow(WindowConfig{}, pWorkspace1);

    // auto pWorkspace2 = createImguiWorkspace("Workspace 2");
    // pApp->createWindow(pWorkspace2);

    pApp->run();
    return 0;
}