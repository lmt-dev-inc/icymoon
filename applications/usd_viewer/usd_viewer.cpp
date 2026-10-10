#include <im3e/guis/guis.h>
#include <im3e/utils/loggers.h>

#include <filesystem>

using namespace im3e;

int main([[maybe_unused]] int argc, char* argv[])
{
    std::filesystem::path appPath{argv[0]};

    auto pLogger = createTerminalLogger();
    auto pApp = createGlfwWindowApplication(*pLogger, WindowApplicationConfig{
                                                          .name = appPath.filename(),
                                                          .isDebugEnabled = true,
                                                      });

    auto pWorkspace = createImguiWorkspace("Workspace");
    pApp->createWindow(WindowConfig{}, pWorkspace);

    pApp->run();
    return 0;
}