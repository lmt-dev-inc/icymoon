#include <im3e/geo/geo.h>
#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>
#include <fmt/std.h>

#include <filesystem>

using namespace im3e;

int main(int argc, char** argv)
{
    auto pLogger = createTerminalLogger();

    std::filesystem::path appRelativePath{argv[0]};
    pLogger->info(fmt::format("Application: {}", appRelativePath.filename()));

    constexpr auto ExpectedArgc = 3U;
    throwIfFalse<std::invalid_argument>(
        argc == ExpectedArgc, fmt::format("Invalid number of arguments passed to application: expected {}, got {}.\n\n"
                                          "Expected Usage:\n"
                                          "\t{} action filePath\n"
                                          "with:\n"
                                          " - action: action to perform. Current options are:\n"
                                          "\t- info: print information about the given file\n"
                                          "\t- rebuild: rebuild overviews of the given file\""
                                          " - filePath: path to the file to process\n",
                                          ExpectedArgc - 1U, argc - 1U, appRelativePath.filename()));

    const std::string action{argv[1]};
    pLogger->info(fmt::format("action: {}", action));

    std::filesystem::path filePath{argv[2]};
    throwIfFalse<std::invalid_argument>(std::filesystem::exists(filePath),
                                        fmt::format("File not found: \"{}\"", filePath));
    pLogger->info(fmt::format("filePath: {}", filePath));

    if (action == "info")
    {
        pLogger->setLevelFilter(LogLevel::Verbose);
        auto pHeightMap = loadHeightMapFromFile(*pLogger, HeightMapFileConfig{.path = filePath, .readOnly = true});
    }
    else if (action == "rebuild")
    {
        auto pHeightMap = loadHeightMapFromFile(*pLogger, HeightMapFileConfig{.path = filePath, .readOnly = false});
        pHeightMap->rebuildPyramid();
    }
    else
    {
        throw std::runtime_error(fmt::format("Unsupported action: {}", action));
    }

    return 0;
}