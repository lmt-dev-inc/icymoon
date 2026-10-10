#include "gdal_geotiff_height_map.h"

#include "gdal_utils.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <fmt/std.h>

#include <limits>

using namespace im3e;

namespace {

template <typename T>
void printMetadata(const ILogger& rLogger, T& rMetadataHolder)
{
    auto pMetadataDomainList = rMetadataHolder.GetMetadataDomainList();
    auto metadataDomainCount = CSLCount(pMetadataDomainList);
    const std::vector<std::string> metadataDomains(pMetadataDomainList, pMetadataDomainList + metadataDomainCount);
    CSLDestroy(pMetadataDomainList);

    if (metadataDomains.empty())
    {
        rLogger.verbose(fmt::format("\t- Metadata: none"));
        return;
    }
    rLogger.verbose(fmt::format("\t- Metadata:"));

    uint32_t index{};
    for (const auto& rMetadataDomain : metadataDomains)
    {
        rLogger.verbose(fmt::format("\t\t- Domain[{}] = \"{}\"", index, rMetadataDomain));
        index++;

        auto pMetadata = rMetadataHolder.GetMetadata(rMetadataDomain.c_str());
        const auto metadataCount = CSLCount(pMetadata);
        std::vector<std::string> metadata(pMetadata, pMetadata + metadataCount);
        for (const auto& rMetadataKeyValue : metadata)
        {
            rLogger.verbose(fmt::format("\t\t\t--> {}", rMetadataKeyValue));
        }
    }
}

auto createGdalDataset(const ILogger& rLogger, const HeightMapFileConfig& rConfig)
{
    // Required to silence warnings from LidarBC files due to mismatch in projection parameters from the file vs
    // ESPG standard. Here we set the option to GEOKEYS to prefer projection parameters from the local file.
    CPLSetConfigOption("GTIFF_SRS_SOURCE", "GEOKEYS");

    const auto fileAccess = rConfig.readOnly ? GA_ReadOnly : GA_Update;
    rLogger.debug(fmt::format("Reading file with {} access", rConfig.readOnly ? "read-only" : "read/write"));

    GDALDatasetUniquePtr pDataset(GDALDataset::FromHandle(GDALOpen(rConfig.path.c_str(), fileAccess)));
    throwIfNull<std::runtime_error>(pDataset, fmt::format("Failed to load height map file \"{}\"", rConfig.path));

    rLogger.debug(fmt::format(R"(Successfully loaded "{}" with driver "{}")", rConfig.path, pDataset->GetDriverName()));

    rLogger.verbose(fmt::format("Information for file:"));
    rLogger.verbose(fmt::format("\t- Size: {}x{}", pDataset->GetRasterXSize(), pDataset->GetRasterYSize()));
    rLogger.verbose(fmt::format("\t- Raster count: {}", pDataset->GetRasterCount()));
    throwIfFalse<std::runtime_error>(pDataset->GetRasterCount() == 1U, "Unexpected number of raster bands, expected 1");

    rLogger.verbose(fmt::format("\t- Layer count: {}", pDataset->GetLayerCount()));
    rLogger.verbose(fmt::format("\t- Projection: {}", pDataset->GetProjectionRef()));

    printMetadata(rLogger, *pDataset);

    return pDataset;
}

auto readSize(GDALRasterBand& rRasterBand)
{
    return glm::u32vec2{rRasterBand.GetXSize(), rRasterBand.GetYSize()};
}

auto readTileSize(GDALRasterBand& rRasterBand)
{
    int blockSizeX, blockSizeY;
    rRasterBand.GetBlockSize(&blockSizeX, &blockSizeY);
    return glm::u32vec2{blockSizeX, blockSizeY};
}

auto calculateTileCount(GDALRasterBand& rRasterBand, const glm::u32vec2& rTileSize)
{
    return glm::u32vec2{
        (rRasterBand.GetXSize() + rTileSize.x - 1) / rTileSize.x,
        (rRasterBand.GetYSize() + rTileSize.y - 1) / rTileSize.y,
    };
}

auto readLodCount(GDALRasterBand& rRasterBand)
{
    // +1 for highest level of details, overviews contain downsampled levels
    return rRasterBand.GetOverviewCount() + 1U;
}

void printRasterBandInfo(const ILogger& rLogger, GDALRasterBand& rRasterBand, std::string_view name)
{
    rLogger.verbose(fmt::format("Information for {}:", name));
    rLogger.verbose(fmt::format("\t- Data type: {}", convertGdalDataTypeToString(rRasterBand.GetRasterDataType())));

    const auto size = readSize(rRasterBand);
    rLogger.verbose(fmt::format("\t- Size: {}x{}", size.x, size.y));

    const auto tileSize = readTileSize(rRasterBand);
    rLogger.verbose(fmt::format("\t- Tile size: {}x{}", tileSize.x, tileSize.y));

    const auto tileCounts = calculateTileCount(rRasterBand, tileSize);
    rLogger.verbose(fmt::format("\t- Tile counts: {}x{}", tileCounts.x, tileCounts.y));

    const auto suggestedAccessPatternStr = convertGdalSuggestedBlockAccessPatternToString(
        rRasterBand.GetSuggestedBlockAccessPattern());
    rLogger.verbose(fmt::format("\t- Suggested access pattern: {}", suggestedAccessPatternStr));

    rLogger.verbose(fmt::format("\t- Scale: {}", rRasterBand.GetScale()));
    rLogger.verbose(fmt::format("\t- Offset: {}", rRasterBand.GetOffset()));
    rLogger.verbose(fmt::format("\t- Unit Type: {}", rRasterBand.GetUnitType()));
    rLogger.verbose(fmt::format("\t- Overview count: {}", rRasterBand.GetOverviewCount()));
    rLogger.verbose(fmt::format("\t- Minimum: {}", rRasterBand.GetMinimum()));
    rLogger.verbose(fmt::format("\t- Maximum: {}", rRasterBand.GetMaximum()));

    int noDataValueFound{};
    const auto noDataValue = rRasterBand.GetNoDataValue(&noDataValueFound);
    if (noDataValueFound)
    {
        rLogger.verbose(fmt::format("\t- No Data Value: {}", noDataValue));
    }
    else
    {
        rLogger.verbose(fmt::format("\t- No Data Value: None"));
    }

    const auto maskFlags = rRasterBand.GetMaskFlags();
    rLogger.verbose(fmt::format("\t- Mask: {}", rRasterBand.GetMaskBand() ? "present" : "none"));
    rLogger.verbose(fmt::format("\t- Mask Flags: {} => {}", maskFlags, convertGdalMaskFlagsToString(maskFlags)));

    printMetadata(rLogger, rRasterBand);
}

auto loadRasterBand(const ILogger& rLogger, GDALDataset& rDataSet)
{
    auto pRasterBand = rDataSet.GetRasterBand(1);
    printRasterBandInfo(rLogger, *pRasterBand, "raster band");
    if (auto pMask = pRasterBand->GetMaskBand())
    {
        printRasterBandInfo(rLogger, *pMask, "raster band mask");
    }

    for (auto i = 0; i < pRasterBand->GetOverviewCount(); i++)
    {
        auto pOverview = pRasterBand->GetOverview(i);

        const std::string overviewName = fmt::format("overview[{}]", i);
        printRasterBandInfo(rLogger, *pOverview, overviewName);
        if (auto pMask = pOverview->GetMaskBand())
        {
            printRasterBandInfo(rLogger, *pMask, fmt::format("{} mask", overviewName));
        }
    }

    return pRasterBand;
}

auto getRasterBandWithLod(GDALRasterBand& rRasterBand, uint32_t lod) -> GDALRasterBand&
{
    return (lod == 0U) ? rRasterBand : *rRasterBand.GetOverview(lod - 1U);
}

class GdalGeoTiffHeightMapTileSampler : public IHeightMapTileSampler
{
public:
    GdalGeoTiffHeightMapTileSampler(GDALRasterBand& rBand, const TileID& rTileID, float scale)
      : m_tileID(rTileID)
      , m_size([&rBand] {
          int sizeX, sizeY;
          rBand.GetBlockSize(&sizeX, &sizeY);
          return glm::u32vec2{static_cast<uint32_t>(sizeX), static_cast<uint32_t>(sizeY)};
      }())
      , m_actualSize([this, &rBand] {
          int actualSizeX, actualSizeY;
          rBand.GetActualBlockSize(m_tileID.x, m_tileID.y, &actualSizeX, &actualSizeY);
          return glm::u32vec2{static_cast<uint32_t>(actualSizeX), static_cast<uint32_t>(actualSizeY)};
      }())
      , m_scale(scale)
      , m_noDataValue([&]() -> std::optional<float> {
          int noDataValueFound{};
          const auto noDataValue = rBand.GetNoDataValue(&noDataValueFound);
          if (noDataValueFound)
          {
              return noDataValue;
          }
          return std::nullopt;
      }())

      , m_pBlock{throwIfNull<std::runtime_error>(rBand.GetLockedBlockRef(m_tileID.x, m_tileID.y),
                                                 fmt::format("Failed to read GDAL height map tile with ID ({}; {}; {})",
                                                             m_tileID.x, m_tileID.y, m_tileID.z)),
                 [](auto* pBlock) { pBlock->DropLock(); }}
      , m_pData{reinterpret_cast<const float*>(m_pBlock->GetDataRef())}

      , m_pMaskBlock([this, &rBand]() -> UniquePtrWithDeleter<GDALRasterBlock> {
          auto pMaskBand = rBand.GetMaskBand();
          if (!pMaskBand)
          {
              return nullptr;
          }
          return UniquePtrWithDeleter<GDALRasterBlock>{pMaskBand->GetLockedBlockRef(m_tileID.x, m_tileID.y),
                                                       [](auto* pBlock) { pBlock->DropLock(); }};
      }())
      , m_pMaskData(m_pMaskBlock ? reinterpret_cast<const uint8_t*>(m_pMaskBlock->GetDataRef()) : nullptr)
    {
    }

    auto at(uint32_t x, uint32_t y) const -> float override { return *(m_pData + y * m_size.x + x); }
    auto at(const glm::u32vec2& rPos) const -> float override { return this->at(rPos.x, rPos.y); }

    auto isValid(uint32_t x, uint32_t y) const -> bool override
    {
        return x < m_actualSize.x && y < m_actualSize.y && m_pMaskData && *(m_pMaskData + y * m_size.x + x) != 0U;
    }

    auto getTileID() const -> const TileID& override { return m_tileID; }
    auto getPos() const -> glm::u32vec2 override { return m_tileID.xy(); }
    auto getSize() const -> const glm::u32vec2& override { return m_size; }
    auto getActualSize() const -> const glm::u32vec2& override { return m_actualSize; }
    auto getScale() const -> float override { return m_scale; }

private:
    const TileID m_tileID;
    const glm::u32vec2 m_size;
    const glm::u32vec2 m_actualSize;
    const float m_scale;
    const std::optional<float> m_noDataValue;

    UniquePtrWithDeleter<GDALRasterBlock> m_pBlock{};
    const float* m_pData;

    UniquePtrWithDeleter<GDALRasterBlock> m_pMaskBlock{};
    const uint8_t* m_pMaskData;
};

}  // namespace

GdalGeoTiffHeightMap::GdalGeoTiffHeightMap(const ILogger& rLogger, HeightMapFileConfig config)
  : m_pLogger(rLogger.createChild(config.path.filename().string()))
  , m_config(std::move(config))
  , m_pGdalInstance(getGdalInstance(rLogger))
  , m_pDataset(createGdalDataset(*m_pLogger, m_config))
  , m_pRasterBand(loadRasterBand(*m_pLogger, *m_pDataset))

  , m_size(readSize(*m_pRasterBand))
  , m_tileSize(readTileSize(*m_pRasterBand))
  , m_lodCount(readLodCount(*m_pRasterBand))

  , m_minHeight(static_cast<float>(m_pRasterBand->GetMinimum()))
  , m_maxHeight(static_cast<float>(m_pRasterBand->GetMaximum()))
{
    m_pLogger->info("Successfully loaded file");
}

namespace {

auto buildPyramidProgressFunction(double progress, const char* pMessage, void* pUserData) -> int
{
    auto pLogger = reinterpret_cast<ILogger*>(pUserData);
    if (pMessage)
    {
        pLogger->info(fmt::format("Building Pyramid - {:.2f}% complete: {})", progress * 100.0, pMessage));
    }
    else
    {
        pLogger->info(fmt::format("Building Pyramid - {:.2f}% complete", progress * 100.0));
    }
    return true;
}

}  // namespace

void GdalGeoTiffHeightMap::rebuildPyramid()
{
    throwIfFalse<std::logic_error>(!m_config.readOnly, "Cannot rebuild pyramid of GeoTIFF while in read-only mode");

    std::vector<int> decimationFactors{};
    glm::u32vec2 currSize{m_pRasterBand->GetXSize(), m_pRasterBand->GetYSize()};
    float currFactor = 1.0F;
    while (currSize.x > m_tileSize.x || currSize.y > m_tileSize.y)
    {
        const auto prevSize = currSize;
        currSize = (currSize + 1U) / 2U;
        currFactor *= 2.0F;
        decimationFactors.emplace_back(currFactor);

        m_pLogger->info(fmt::format("level {}: {}x{} => {}x{}, factor = {}", decimationFactors.size(), prevSize.x,
                                    prevSize.y, currSize.x, currSize.y, decimationFactors.back()));
    }

    const int targetBandIndex = 1U;

    const auto returnCode = m_pDataset->BuildOverviews("BILINEAR", decimationFactors.size(), decimationFactors.data(),
                                                       1U, &targetBandIndex, &buildPyramidProgressFunction,
                                                       m_pLogger.get());
    throwIfFalse<std::runtime_error>(returnCode == CE_None, "Failed to build pyramid of GeoTIFF");
}

auto GdalGeoTiffHeightMap::getTileSampler(const TileID& rTileID) -> std::unique_ptr<IHeightMapTileSampler>
{
    throwIfFalse<std::invalid_argument>(rTileID.z < m_lodCount,
                                        fmt::format("Invalid input LOD: {} > max {}", rTileID.z, m_lodCount));
    auto& rBand = getRasterBandWithLod(*m_pRasterBand, rTileID.z);
    const auto scale = std::pow(2.0F, rTileID.z);
    return std::make_unique<GdalGeoTiffHeightMapTileSampler>(rBand, rTileID, scale);
}

auto GdalGeoTiffHeightMap::getTileSampler(const glm::u32vec2& rTilePos, uint32_t lod)
    -> std::unique_ptr<IHeightMapTileSampler>
{
    throwIfFalse<std::invalid_argument>(lod < m_lodCount,
                                        fmt::format("Invalid input LOD: {} > max {}", lod, m_lodCount));
    auto& rBand = getRasterBandWithLod(*m_pRasterBand, lod);
    const auto scale = std::pow(2.0F, lod);
    return std::make_unique<GdalGeoTiffHeightMapTileSampler>(rBand, TileID{rTilePos.x, rTilePos.y, lod}, scale);
}

auto GdalGeoTiffHeightMap::getTileCount(uint32_t lod) const -> glm::u32vec2
{
    throwIfFalse<std::invalid_argument>(lod < m_lodCount,
                                        fmt::format("Invalid input LOD: {} > max {}", lod, m_lodCount));
    auto& rBand = getRasterBandWithLod(*m_pRasterBand, lod);
    return calculateTileCount(rBand, m_tileSize);
}

auto im3e::loadHeightMapFromFile(const ILogger& rLogger, HeightMapFileConfig config) -> std::unique_ptr<IHeightMap>
{
    return std::make_unique<GdalGeoTiffHeightMap>(rLogger, std::move(config));
}
