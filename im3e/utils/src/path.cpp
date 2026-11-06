#include "path.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>

#include <algorithm>

using namespace im3e;

Path::Path(const char* pPath)
  : Path(std::string_view{pPath})
{
}

Path::Path(std::string_view path)
{
    throwIfFalse<std::invalid_argument>(path.starts_with('/'), fmt::format("Path must be absolute: \"{}\"", path));
    size_t start = 0U;
    while (start < path.size())
    {
        const auto end = std::min(path.find('/', start), path.size());
        if (end > start)
        {
            m_components.emplace_back(path.substr(start, end - start));
        }
        start = end + 1U;
    }
}

auto Path::operator/(std::string_view name) const -> Path
{
    throwIfFalse<std::invalid_argument>(!name.empty() && name.find('/') == std::string_view::npos,
                                        fmt::format("Invalid path component: \"{}\"", name));
    auto path = *this;
    path.m_components.emplace_back(name);
    return path;
}

auto Path::getName() const -> std::string_view
{
    return isRoot() ? std::string_view{} : std::string_view{m_components.back()};
}

auto Path::getParent() const -> Path
{
    auto parent = *this;
    if (!parent.isRoot())
    {
        parent.m_components.pop_back();
    }
    return parent;
}

auto Path::toString() const -> std::string
{
    if (isRoot())
    {
        return "/";
    }
    std::string str;
    for (const auto& rComponent : m_components)
    {
        str += '/';
        str += rComponent;
    }
    return str;
}
