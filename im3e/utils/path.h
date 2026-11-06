#pragma once

#include <compare>
#include <string>
#include <string_view>
#include <vector>

namespace im3e {

/// @brief Absolute path identifying a node in a tree of named nodes, e.g. "/camera/view".
/// Unlike std::filesystem::path, the separator is always "/" whatever the OS, and ":" has no special meaning.
class Path
{
public:
    /// @brief Root path: "/".
    Path() = default;
    Path(const char* pPath);
    /// @throws std::invalid_argument if the path does not start with "/". Empty components are ignored, so "/a//b" is
    /// the same as "/a/b".
    Path(std::string_view path);

    /// @brief Appends a component to the path.
    /// @throws std::invalid_argument if the name is empty or contains "/".
    [[nodiscard]] auto operator/(std::string_view name) const -> Path;

    [[nodiscard]] auto operator<=>(const Path&) const = default;

    [[nodiscard]] auto isRoot() const -> bool { return m_components.empty(); }

    /// @brief Last component of the path, empty for the root.
    [[nodiscard]] auto getName() const -> std::string_view;

    /// @brief Parent of the path. The parent of the root is the root.
    [[nodiscard]] auto getParent() const -> Path;

    [[nodiscard]] auto toString() const -> std::string;

private:
    std::vector<std::string> m_components;
};

}  // namespace im3e
