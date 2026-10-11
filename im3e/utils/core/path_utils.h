#pragma once

#include <filesystem>
#include <string>
#include <string_view>

// Conversions between paths and UTF-8 strings, for libraries that take or return file names as UTF-8 char strings
// (GDAL, ImGui, whereami...). Unlike path::string() and path{std::string}, they don't depend on the code page of the
// process on Windows, where paths are UTF-16.
namespace im3e::path {

auto toUtf8(const std::filesystem::path& rPath) -> std::string;
auto fromUtf8(std::string_view utf8) -> std::filesystem::path;

}  // namespace im3e::path
