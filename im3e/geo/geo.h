#pragma once

#include <im3e/geo/height_map.h>
#include <im3e/utils/loggers.h>
#include <im3e/utils/math_utils.h>
#include <im3e/utils/view_frustum.h>

#include <glm/glm.hpp>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>

namespace im3e {

struct HeightMapFileConfig
{
    std::filesystem::path path;
    bool readOnly = true;
};
auto loadHeightMapFromFile(const ILogger& rLogger, HeightMapFileConfig config) -> std::unique_ptr<IHeightMap>;

struct HeightMapQuadTreeNode
{
    TileID tileID;

    glm::vec3 minWorldPos;
    glm::vec3 maxWorldPos;

    std::array<std::shared_ptr<HeightMapQuadTreeNode>, 4U> pChildren;

    /// @brief Find tiles that are visible within the given view frustum.
    /// @param[in] rViewFrustum View frustum
    /// @param[in] lod Level of Detail that the function should return
    /// @return List of vec3 defined as (x, y, l) with (x, y) the tile position at the level of detail l.
    auto findVisible(const ViewFrustum& rViewFrustum, uint32_t lod) const -> std::vector<TileID>;
};
auto generateHeightMapQuadTree(const IHeightMap& rHeightMap) -> std::shared_ptr<HeightMapQuadTreeNode>;

}  // namespace im3e