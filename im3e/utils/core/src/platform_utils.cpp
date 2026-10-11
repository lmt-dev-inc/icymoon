#include "platform_utils.h"

#include "path_utils.h"

#include <whereami.h>

#include <string>

using namespace im3e;

auto im3e::getCurrentExecutableFolder() -> std::filesystem::path
{
    auto length = wai_getExecutablePath(nullptr, 0, nullptr);

    // whereami returns UTF-8 paths
    std::string executablePath(length, ' ');
    wai_getExecutablePath(executablePath.data(), length, &length);
    return path::fromUtf8(executablePath).parent_path();
}
