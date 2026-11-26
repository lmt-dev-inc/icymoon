#include <im3e/guis/guis.h>
#include <im3e/render/vulkan/vulkan_render_engine.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>
#include <CLI/CLI.hpp>

#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace im3e;

namespace {

struct Arguments
{
    bool debugEnabled{};
    LogLevel logLevel{};
};

struct ParseResults
{
    std::optional<Arguments> arguments;
    int exitCode{};
};

auto parseCliArguments(int argc, char* argv[]) -> ParseResults
{
    CLI::App app("IcyMoon Viewer", "im3e_viewer");
    argv = app.ensure_utf8(argv);

#if defined(NDEBUG)
    bool debugEnabled = false;
#else
    bool debugEnabled = true;
#endif
    app.add_flag("-d,--debug,!--no-debug", debugEnabled,
                 "Enable debug mode including debug level logs and Vulkan validation layer");

    bool verboseEnabled{false};
    app.add_flag("-v,--verbose", verboseEnabled, "Enable verbose log level");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& error)
    {
        // Prints the help, or the error with the usage, and returns the matching exit code
        return ParseResults{.exitCode = app.exit(error)};
    }

    return ParseResults{
        .arguments =
            Arguments{
                .debugEnabled = debugEnabled,
                .logLevel = verboseEnabled  //
                                ? LogLevel::Verbose
                                : (debugEnabled ? LogLevel::Debug : LogLevel::Info),
            },
        .exitCode = 0,
    };
}

auto toString(LogLevel level) -> std::string
{
    switch (level)
    {
        case LogLevel::Error: return "Error";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Info: return "Info";
        case LogLevel::Debug: return "Debug";
        case LogLevel::Verbose: return "Verbose";
    }
    throw std::invalid_argument("Unrecognized LogLevel");
}

}  // namespace

auto main(int argc, char* argv[]) -> int
{
    try
    {
        auto [argumentsOpt, exitCode] = parseCliArguments(argc, argv);
        if (exitCode || !argumentsOpt)
        {
            return exitCode;
        }
        const auto& arguments = argumentsOpt.value();

        auto logger = createTerminalLogger();
        logger->setLevelFilter(arguments.logLevel);
        logger->info("IcyMoon Viewer");
        logger->info(fmt::format("Debug enabled: {}", arguments.debugEnabled));
        logger->info(fmt::format("Log level: {}", toString(arguments.logLevel)));

        auto app = createGlfwWindowApplication(*logger, WindowApplicationConfig{
                                                            .name = "Viewer",
                                                            .isDebugEnabled = arguments.debugEnabled,
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
    }
    catch (const std::exception& e)
    {
        fmt::println(stderr, "Fatal error: {}", e.what());
        return -1;
    }
    return 0;
}
