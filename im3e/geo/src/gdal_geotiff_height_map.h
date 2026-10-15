#pragma once

#include "gdal_instance.h"
#include "geo.h"

#include <im3e/api/height_map.h>
#include <im3e/utils/core/path_utils.h>
#include <im3e/utils/loggers.h>

#include <gdal_priv.h>

#include <memory>
#include <numeric>

namespace im3e {

class GdalGeoTiffHeightMap : public IHeightMap
{
public:
    GdalGeoTiffHeightMap(const ILogger& rLogger, HeightMapFileConfig config);

    void rebuildPyramid() override;

    auto getTileSampler(const TileID& rTileID) -> std::unique_ptr<IHeightMapTileSampler> override;
    auto getTileSampler(const glm::u32vec2& rTilePos, uint32_t level)
        -> std::unique_ptr<IHeightMapTileSampler> override;

    auto getName() const -> std::string override { return path::toUtf8(m_config.path.stem()); }
    auto getSize() const -> glm::u32vec2 override { return m_size; }
    auto getTileSize() const -> glm::u32vec2 override { return m_tileSize; }
    auto getTileCount(uint32_t lod) const -> glm::u32vec2 override;
    auto getLodCount() const -> uint32_t override { return m_lodCount; }
    auto getMinHeight() const -> float override { return m_minHeight; }
    auto getMaxHeight() const -> float override { return m_maxHeight; }

private:
    std::unique_ptr<ILogger> m_pLogger;
    const HeightMapFileConfig m_config;

    std::shared_ptr<IGdalInstance> m_pGdalInstance;
    GDALDatasetUniquePtr m_pDataset;
    GDALRasterBand* m_pRasterBand;

    const glm::u32vec2 m_size;
    const glm::u32vec2 m_tileSize;
    const uint32_t m_lodCount;

    const float m_minHeight = std::numeric_limits<float>::min();
    const float m_maxHeight = std::numeric_limits<float>::max();
};

}  // namespace im3e