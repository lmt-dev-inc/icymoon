#include "path_utils.h"

using namespace im3e;

auto im3e::path::toUtf8(const std::filesystem::path& rPath) -> std::string
{
    const auto utf8 = rPath.u8string();
    return {utf8.begin(), utf8.end()};
}

auto im3e::path::fromUtf8(std::string_view utf8) -> std::filesystem::path
{
    return std::u8string{utf8.begin(), utf8.end()};
}
