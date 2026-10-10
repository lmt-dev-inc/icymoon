#include "gdal_dem_loader.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <fmt/std.h>

using namespace im3e;
using namespace std::string_view_literals;

namespace {

auto convertGdalDataTypeToString(GDALDataType dataType)
{
    switch (dataType)
    {
        case GDT_Unknown: return "Unknown"sv;
        case GDT_Byte: return "Byte"sv;
        case GDT_Int8: return "Int8"sv;
        case GDT_UInt16: return "UInt16"sv;
        case GDT_Int16: return "Int16"sv;
        case GDT_UInt32: return "UInt32"sv;
        case GDT_Int32: return "Int32"sv;
        case GDT_UInt64: return "UInt64"sv;
        case GDT_Int64: return "Int64"sv;
        case GDT_Float32: return "Float32"sv;
        case GDT_Float64: return "Float64"sv;
        case GDT_CInt16: return "CInt16"sv;
        case GDT_CInt32: return "CInt32"sv;
        case GDT_CFloat32: return "CFloat32"sv;
        case GDT_CFloat64: return "CFloat64"sv;
        default: break;
    }
    return "Undefined"sv;
}

auto printDemMetadata(const ILogger& rLogger, GDALDataset& rDataset)
{
    auto pMetadataDomainList = rDataset.GetMetadataDomainList();
    auto metadataDomainCount = CSLCount(pMetadataDomainList);
    const std::vector<std::string> metadataDomains(pMetadataDomainList, pMetadataDomainList + metadataDomainCount);
    CSLDestroy(pMetadataDomainList);

    uint32_t index{};
    for (const auto& rMetadataDomain : metadataDomains)
    {
        rLogger.info(fmt::format(R"(Domain[{}] = "{}")", index, rMetadataDomain));
        index++;

        auto pMetadata = rDataset.GetMetadata(rMetadataDomain.c_str());
        const auto metadataCount = CSLCount(pMetadata);
        std::vector<std::string> metadata(pMetadata, pMetadata + metadataCount);
        for (const auto& rMetadataKeyValue : metadata)
        {
            rLogger.info(fmt::format("--> {}", rMetadataKeyValue));
        }
    }
}

auto loadRasterBand(const ILogger& rLogger, GDALDataset& rDataSet)
{
    auto pRasterBand = rDataSet.GetRasterBand(1);
    rLogger.info(fmt::format("Data type is {}", convertGdalDataTypeToString(pRasterBand->GetRasterDataType())));
    rLogger.info(fmt::format("Raster band size is {}x{}", pRasterBand->GetXSize(), pRasterBand->GetYSize()));

    int blockSizeX, blockSizeY;
    pRasterBand->GetBlockSize(&blockSizeX, &blockSizeY);
    rLogger.info(fmt::format("Raster block size is {}x{}", blockSizeX, blockSizeY));

    const auto blockCountX = (pRasterBand->GetXSize() + blockSizeX - 1) / blockSizeX;
    const auto blockCountY = (pRasterBand->GetYSize() + blockSizeY - 1) / blockSizeY;
    rLogger.info(fmt::format("Raster block counts are {}x{}", blockCountX, blockCountY));

    rLogger.info(fmt::format("Raster scale: {}", pRasterBand->GetScale()));
    rLogger.info(fmt::format("Raster overview count: {}", pRasterBand->GetOverviewCount()));

    rLogger.info(fmt::format("Raster Minimum: {} / Maximum: {}", pRasterBand->GetMinimum(), pRasterBand->GetMaximum()));

    return pRasterBand;
}

auto getBlockSizeFromGdalRaster(GDALRasterBand& rRasterBand)
{
    int blockWidth, blockHeight;
    rRasterBand.GetBlockSize(&blockWidth, &blockHeight);
    return glm::u32vec2{static_cast<uint32_t>(blockWidth), static_cast<uint32_t>(blockHeight)};
}

}  // namespace

GdalDemLoader::GdalDemLoader(const ILogger& rLogger, const std::filesystem::path& rDemFilePath)
  : m_pLogger(rLogger.createChild(fmt::format("GDAL DEM Loader: {}", rDemFilePath.filename())))
{
    GDALAllRegister();
    CPLSetConfigOption("GTIFF_SRS_SOURCE", "GEOKEYS");

    m_pDataset.reset(GDALDataset::FromHandle(GDALOpen(rDemFilePath.c_str(), GA_ReadOnly)));
    throwIfNull<std::runtime_error>(m_pDataset, "Could not load GDAL dataset");
    m_pLogger->info(
        fmt::format(R"(Successfully loaded "{}" with driver "{}")", rDemFilePath, m_pDataset->GetDriverName()));
    m_pLogger->info(fmt::format("Raster size is {}x{}", m_pDataset->GetRasterXSize(), m_pDataset->GetRasterYSize()));
    m_pLogger->info(fmt::format("Raster count is {}", m_pDataset->GetRasterCount()));
    m_pLogger->info(fmt::format("Layer count is {}", m_pDataset->GetLayerCount()));

    m_pLogger->info(fmt::format("Projection is {}", m_pDataset->GetProjectionRef()));

    printDemMetadata(*m_pLogger, *m_pDataset);

    m_pRasterBand = loadRasterBand(*m_pLogger, *m_pDataset);
    m_blockSize = getBlockSizeFromGdalRaster(*m_pRasterBand);
    m_blockCount = glm::u32vec2{
        (static_cast<uint32_t>(m_pRasterBand->GetXSize()) + m_blockSize[0] - 1) / m_blockSize[0],
        (static_cast<uint32_t>(m_pRasterBand->GetYSize()) + m_blockSize[1] - 1) / m_blockSize[1],
    };
    int hasMinValue{};
    m_minValue = m_pRasterBand->GetMinimum(&hasMinValue);
    if (!hasMinValue)
    {
        m_minValue = 0.0F;
    }
    m_blockScale = glm::vec3(m_blockSize.x * static_cast<float>(m_pRasterBand->GetScale()));
}

namespace {

inline auto makeGdalFloatBlock(GDALRasterBand& rBand, uint32_t blockPosX, uint32_t blockPosY, float minV, float scale)
{
    auto pBlock = rBand.GetLockedBlockRef(blockPosX, blockPosY);
    auto pData = reinterpret_cast<float*>(pBlock->GetDataRef());

    int actualBlockSizeX, actualBlockSizeY;
    rBand.GetActualBlockSize(blockPosX, blockPosY, &actualBlockSizeX, &actualBlockSizeY);

    int blockSizeX, blockSizeY;
    rBand.GetBlockSize(&blockSizeX, &blockSizeY);

    return std::make_unique<DemBlockSampler<float>>(
        std::unique_ptr<const float, std::function<void(const float*)>>(pData, [pBlock](auto*) { pBlock->DropLock(); }),
        glm::u32vec2{blockPosX, blockPosY}, glm::u32vec2{blockSizeX, blockSizeY},
        glm::u32vec2{actualBlockSizeX, actualBlockSizeY}, minV, scale);
}

}  // namespace

auto GdalDemLoader::createBlockSamplers(const glm::u32vec2& rBlockPos) -> DemBlockSamplers<float>
{
    const auto minV = m_minValue;
    const auto scale = m_blockScale.y;

    DemBlockSamplers<float> samplers{
        .pBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x, rBlockPos.y, minV, scale),
    };
    if (rBlockPos.y > 0)
    {
        samplers.pTopBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x, rBlockPos.y - 1, minV, scale);
        if (rBlockPos.x > 0)
        {
            samplers.pTopLeftBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x - 1, rBlockPos.y - 1, minV, scale);
        }
        if (rBlockPos.x < m_blockCount.x - 1)
        {
            samplers.pTopRightBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x + 1, rBlockPos.y - 1, minV, scale);
        }
    }
    if (rBlockPos.y < m_blockCount.y - 1)
    {
        samplers.pBottomBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x, rBlockPos.y + 1, minV, scale);
        if (rBlockPos.x > 0)
        {
            samplers.pBottomLeftBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x - 1, rBlockPos.y + 1, minV,
                                                           scale);
        }
        if (rBlockPos.x < m_blockCount.x - 1)
        {
            samplers.pBottomRightBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x + 1, rBlockPos.y + 1, minV,
                                                            scale);
        }
    }
    if (rBlockPos.x > 0)
    {
        samplers.pLeftBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x - 1, rBlockPos.y, minV, scale);
    }
    if (rBlockPos.x < m_blockCount.x - 1)
    {
        samplers.pRightBlock = makeGdalFloatBlock(*m_pRasterBand, rBlockPos.x + 1, rBlockPos.y, minV, scale);
    }

    return samplers;
}
