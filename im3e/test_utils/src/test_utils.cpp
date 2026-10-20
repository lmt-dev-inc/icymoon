#include "test_utils.h"

#include <im3e/utils/core/path_utils.h>
#include <im3e/utils/core/platform_utils.h>

#include <fmt/format.h>

using namespace im3e;

auto im3e::generateTestOutputFilePath(std::string_view suiteName, std::string_view name, std::string_view extension)
    -> std::filesystem::path
{
    const auto suiteFolder = getCurrentExecutableFolder() / path::fromUtf8(suiteName);
    std::filesystem::create_directories(suiteFolder);
    return suiteFolder / path::fromUtf8(fmt::format("{}.{}", name, extension));
}