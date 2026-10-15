#include "anari_dem_surface_generator.h"

#include "gdal_dem_loader.h"

#include <im3e/utils/core/throw_utils.h>

using namespace im3e;

namespace {

[[maybe_unused]] auto calculateVertexCount(const glm::u32vec2& rBlockSize)
{
    throwIfFalse<std::invalid_argument>(rBlockSize.x != 0U && rBlockSize.y != 0U,
                                        "Cannot generate a DEM vertex buffer with a block size of (0, 0)");

    // Add one column of vertices on the left/top and 2 columns on the right/bottom to allow for normal calculation in
    // shaders. An extra column is needed on the right/bottom because triangles make use of one extra column to fill
    // any gap formed between mesh tiles:
    return (rBlockSize.x + 3U) * (rBlockSize.y + 3U);
}

struct SamplerHelper
{
    SamplerHelper(const DemBlockSamplers<float>& rSamplers)
      : rBlock(*rSamplers.pBlock)

      , rTopBlock(rSamplers.pTopBlock ? *rSamplers.pTopBlock : *rSamplers.pBlock)
      , topBlockY(rSamplers.pTopBlock ? (rTopBlock.getActualBlockSize().y - 1U) : 0U)

      , rTopLeftBlock(rSamplers.pTopLeftBlock ? *rSamplers.pTopLeftBlock : *rSamplers.pBlock)
      , topLeftBlockPos{
            rSamplers.pTopLeftBlock ? (rTopLeftBlock.getActualBlockSize().x - 1U) : 0U,
            rSamplers.pTopLeftBlock ? (rTopLeftBlock.getActualBlockSize().y - 1U) : 0U,
        }

      , rLeftBlock(rSamplers.pLeftBlock ? *rSamplers.pLeftBlock : *rSamplers.pBlock)
      , leftBlockX(rSamplers.pLeftBlock ? (rLeftBlock.getActualBlockSize().x - 1U) : 0U)

      , rBottomLeftBlock(rSamplers.pBottomLeftBlock ? *rSamplers.pBottomLeftBlock : *rSamplers.pBlock)
      , bottomLeftBlockPos0{
            rSamplers.pBottomLeftBlock ? (rBottomLeftBlock.getActualBlockSize().x - 1U) : 0U,
            rSamplers.pBottomLeftBlock ? 0U : (rBottomLeftBlock.getActualBlockSize().y - 1U),
        }
      , bottomLeftBlockPos1{
            bottomLeftBlockPos0.x,
            rSamplers.pBottomLeftBlock ? 1U : (rBottomLeftBlock.getActualBlockSize().y - 1U),
        }

      , rBottomBlock(rSamplers.pBottomBlock ? *rSamplers.pBottomBlock : *rSamplers.pBlock)
      , bottomBlockY0(rSamplers.pBottomBlock ? 0U : (rBottomBlock.getActualBlockSize().y - 1U))
      , bottomBlockY1(rSamplers.pBottomBlock ? 1U : (rBottomBlock.getActualBlockSize().y - 1U))

      , rBottomRightBlock(rSamplers.pBottomRightBlock ? *rSamplers.pBottomRightBlock : *rSamplers.pBlock)
      , bottomRightX0{rSamplers.pBottomRightBlock ? 0U : (rBottomRightBlock.getActualBlockSize().x - 1U)}
      , bottomRightY0{rSamplers.pBottomRightBlock ? 0U : (rBottomRightBlock.getActualBlockSize().y - 1U)}
      , bottomRightX1{rSamplers.pBottomRightBlock ? 1U : (rBottomRightBlock.getActualBlockSize().x - 1U)}
      , bottomRightY1{rSamplers.pBottomRightBlock ? 1U : (rBottomRightBlock.getActualBlockSize().y - 1U)}

      , rRightBlock(rSamplers.pRightBlock ? *rSamplers.pRightBlock : *rSamplers.pBlock)
      , rightBlockX0(rSamplers.pRightBlock ? 0U : (rRightBlock.getActualBlockSize().x - 1U))
      , rightBlockX1(rSamplers.pRightBlock ? 1U : (rRightBlock.getActualBlockSize().x - 1U))

      , rTopRightBlock(rSamplers.pTopRightBlock ? *rSamplers.pTopRightBlock : *rSamplers.pBlock)
      , topRightBlockPos0{
            rSamplers.pTopRightBlock ? 0U : (rTopRightBlock.getActualBlockSize().x - 1U),
            rSamplers.pTopRightBlock ? (rTopRightBlock.getActualBlockSize().y - 1U) : 0U,
      }
      , topRightBlockPos1{
            rSamplers.pTopRightBlock ? 1U : (rTopRightBlock.getActualBlockSize().x - 1U),
            topRightBlockPos0.y,
      }
    {
    }

    const DemBlockSampler<float>& rBlock;

    const DemBlockSampler<float>& rTopBlock;
    const uint32_t topBlockY;

    const DemBlockSampler<float>& rTopLeftBlock;
    const glm::u32vec2 topLeftBlockPos;

    const DemBlockSampler<float>& rLeftBlock;
    const uint32_t leftBlockX;

    const DemBlockSampler<float>& rBottomLeftBlock;
    const glm::u32vec2 bottomLeftBlockPos0;
    const glm::u32vec2 bottomLeftBlockPos1;

    const DemBlockSampler<float>& rBottomBlock;
    const uint32_t bottomBlockY0;
    const uint32_t bottomBlockY1;

    const DemBlockSampler<float>& rBottomRightBlock;
    const uint32_t bottomRightX0;
    const uint32_t bottomRightY0;
    const uint32_t bottomRightX1;
    const uint32_t bottomRightY1;

    const DemBlockSampler<float>& rRightBlock;
    const uint32_t rightBlockX0;
    const uint32_t rightBlockX1;

    const DemBlockSampler<float>& rTopRightBlock;
    const glm::u32vec2 topRightBlockPos0;
    const glm::u32vec2 topRightBlockPos1;
};

[[maybe_unused]] void generateVertexBuffer(const DemBlockSamplers<float>& rSamplers,
                                           std::vector<glm::vec3>& rVertexBuffer)
{
    const auto& rActualBlockSize = rSamplers.pBlock->getActualBlockSize();
    throwIfFalse<std::invalid_argument>(rActualBlockSize.x > 0U && rActualBlockSize.y > 0U,
                                        "Cannot generate mesh with height map of size 0x0 or lower");

    const SamplerHelper helper(rSamplers);

    auto* pVertexData = rVertexBuffer.data();
    const auto& rBlockSize = rSamplers.pBlock->getBlockSize();
    const glm::vec2 flSize(rBlockSize);
    constexpr glm::vec3 NaNVec{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN()};

    // Top Row:
    {
        // Top-Left Corner:
        {
            const auto height = helper.rTopLeftBlock.at(helper.topLeftBlockPos);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{-1.0F, height, -1.0F} : NaNVec;
        }

        // Centre of Row:
        for (uint32_t x = 0U; x < rActualBlockSize.x; x++)
        {
            const auto height = helper.rTopBlock.at(x, helper.topBlockY);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{x, height, -1.0F} : NaNVec;
        }

        // Top-Right Corner:
        {
            auto xF = static_cast<float>(rActualBlockSize.x);
            auto height = helper.rTopRightBlock.at(helper.topRightBlockPos0);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, -1.0F} : NaNVec;

            xF += 1.0F;
            height = helper.rTopRightBlock.at(helper.topRightBlockPos1);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, -1.0F} : NaNVec;
        }

        // Outside of partial block:
        for (uint32_t x = rActualBlockSize.x; x < rBlockSize.x; x++)
        {
            *(pVertexData++) = NaNVec;
        }
    }

    // Central Rows:
    for (uint32_t y = 0U; y < rActualBlockSize.y; y++)
    {
        // Left Edge:
        {
            const auto height = helper.rLeftBlock.at(helper.leftBlockX, y);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{-1.0F, height, y} : NaNVec;
        }

        // Centre of Row:
        for (uint32_t x = 0U; x < rActualBlockSize.x; x++)
        {
            const auto height = helper.rBlock.at(x, y);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{x, height, y} : NaNVec;
        }

        // Right Edge:
        {
            auto xF = static_cast<float>(rActualBlockSize.x);
            auto height = helper.rRightBlock.at(helper.rightBlockX0, y);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, y} : NaNVec;

            xF += 1.0F;
            height = helper.rRightBlock.at(helper.rightBlockX1, y);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, y} : NaNVec;
        }

        // Outside of partial block:
        for (uint32_t x = rActualBlockSize.x; x < rBlockSize.x; x++)
        {
            *(pVertexData++) = NaNVec;
        }
    }

    // Last Rows:
    for (uint32_t yT = 0U; yT <= 1U; yT++)
    {
        const auto yF = static_cast<float>(rActualBlockSize.y + yT);

        // Bottom-Left Corner:
        {
            auto height = helper.rBottomLeftBlock.at(yT == 0U ? helper.bottomLeftBlockPos0
                                                              : helper.bottomLeftBlockPos1);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{-1.0F, height, yF} : NaNVec;
        }

        // Centre of Row:
        for (uint32_t x = 0U; x < rActualBlockSize.x; x++)
        {
            auto height = helper.rBottomBlock.at(x, yT == 0U ? helper.bottomBlockY0 : helper.bottomBlockY1);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{x, height, yF} : NaNVec;
        }

        // Bottom-Right Corner:
        {
            auto xF = static_cast<float>(rActualBlockSize.x);
            auto height = helper.rBottomRightBlock.at(helper.bottomRightX0,
                                                      yT == 0U ? helper.bottomRightY0 : helper.bottomRightY1);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, yF} : NaNVec;

            xF += 1.0F;
            height = helper.rBottomRightBlock.at(helper.bottomRightX1,
                                                 yT == 0U ? helper.bottomRightY0 : helper.bottomRightY1);
            *(pVertexData++) = height >= 0.0F ? glm::vec3{xF, height, yF} : NaNVec;
        }

        // Outside of partial block:
        for (uint32_t x = rActualBlockSize.x; x < rBlockSize.x; x++)
        {
            *(pVertexData++) = NaNVec;
        }
    }

    // Outside of partial block:
    for (uint32_t y = rActualBlockSize.y; y < rBlockSize.y; y++)
    {
        for (uint32_t x = 0U; x < rBlockSize.x + 3U; x++)
        {
            *(pVertexData++) = NaNVec;
        }
    }
}

[[maybe_unused]] auto generateIndexBuffer(const glm::u32vec2& rBlockSize) -> std::vector<glm::u32vec3>
{
    // Here, vertex indices are calculated with (x, y) being the vertex coordinates within the block, excluding edges
    // we add to the vertex buffer. As a result, the indices must be calculated knowing there are 2 extra vertices per
    // dimension and a (1, 1) offset must be applied to (x, y).
    const auto vertexBufferWidth = rBlockSize.x + 3U;

    const auto triangleCount = rBlockSize.x * rBlockSize.y * 2U;
    std::vector<glm::u32vec3> indexBuffer(triangleCount);
    auto* pIndexData = reinterpret_cast<uint32_t*>(indexBuffer.data());

    for (uint32_t y = 0U; y < rBlockSize.y; y++)
    {
        for (uint32_t x = 0U; x < rBlockSize.x; x++)
        {
            *(pIndexData++) = vertexBufferWidth * (y + 1U) + (x + 1U);
            *(pIndexData++) = vertexBufferWidth * (y + 2U) + (x + 1U);
            *(pIndexData++) = vertexBufferWidth * (y + 1U) + (x + 2U);

            *(pIndexData++) = vertexBufferWidth * (y + 2U) + (x + 1U);
            *(pIndexData++) = vertexBufferWidth * (y + 2U) + (x + 2U);
            *(pIndexData++) = vertexBufferWidth * (y + 1U) + (x + 2U);
        }
    }
    return indexBuffer;
}

auto createGeometry(const ILogger& rLogger, ANARIDevice anDevice)
{
    auto pDemLoader = std::make_unique<GdalDemLoader>(rLogger,
                                                      "/mnt/data/dev/assets/lidar_bc/bc_092g064_xli1m_utm10_2020.tif");

    auto anGeometry = anariNewGeometry(anDevice, "triangle");
    auto pGeometry = std::shared_ptr<anari::api::Geometry>(anGeometry,
                                                           [anDevice, pLogger = &rLogger](auto* anGeometry) {
                                                               anariRelease(anDevice, anGeometry);
                                                               pLogger->debug("Released geometry");
                                                           });

    // Vertex positions:
    {
        std::vector<glm::vec3> vertexBuffer(calculateVertexCount(pDemLoader->getBlockSize()));
        generateVertexBuffer(pDemLoader->createBlockSamplers(glm::u32vec2{0U, 0U}), vertexBuffer);

        auto anArray = anariNewArray1D(anDevice, vertexBuffer.data(), nullptr, nullptr, ANARI_FLOAT32_VEC3,
                                       vertexBuffer.size());

        /*constexpr array<array<float, 3U>, 4U> Vertices{
            array<float, 3U>{-1.0F, 0.0F, 1.0F},
            array<float, 3U>{-1.0F, 0.0F, -1.0F},
            array<float, 3U>{1.0F, 0.0F, 1.0F},
            array<float, 3U>{1.0F, 0.0F, -1.0F},
        };
        auto anArray = anariNewArray1D(anDevice, Vertices.data(), nullptr, nullptr, ANARI_FLOAT32_VEC3,
                                       Vertices.size());*/

        anariCommitParameters(anDevice, anArray);
        anariSetParameter(anDevice, anGeometry, "vertex.position", ANARI_ARRAY1D, &anArray);
        anariRelease(anDevice, anArray);
    }

    // Vertex indices:
    {
        const auto indexBuffer = generateIndexBuffer(pDemLoader->getBlockSize());
        auto anArray = anariNewArray1D(anDevice, indexBuffer.data(), nullptr, nullptr, ANARI_UINT32_VEC3,
                                       indexBuffer.size());

        /*constexpr array<array<int32_t, 3U>, 2U> VertexIndices{
            array<int32_t, 3U>{0U, 1U, 2U},
            array<int32_t, 3U>{1U, 2U, 3U},
        };
        auto anArray = anariNewArray1D(anDevice, VertexIndices.data(), nullptr, nullptr, ANARI_UINT32_VEC3,
                                       VertexIndices.size());*/

        anariCommitParameters(anDevice, anArray);
        anariSetParameter(anDevice, anGeometry, "primitive.index", ANARI_ARRAY1D, &anArray);
        anariRelease(anDevice, anArray);
    }

    anariCommitParameters(anDevice, anGeometry);
    rLogger.debug("Created Geometry");
    return pGeometry;
}

}  // namespace

auto AnariDemSurfaceGenerator::generate(const ILogger& rLogger, ANARIDevice anDevice)
    -> std::shared_ptr<anari::api::Surface>
{
    auto pGeometry = createGeometry(rLogger, anDevice);
    auto anGeometry = pGeometry.get();

    auto anMaterial = anariNewMaterial(anDevice, "matte");
    glm::vec3 materialColor{0.8F, 0.2F, 0.2F};
    anariSetParameter(anDevice, anMaterial, "color", ANARI_FLOAT32_VEC3, &materialColor);
    anariCommitParameters(anDevice, anMaterial);

    auto anSurface = anariNewSurface(anDevice);
    anariSetParameter(anDevice, anSurface, "geometry", ANARI_GEOMETRY, &anGeometry);
    anariSetParameter(anDevice, anSurface, "material", ANARI_MATERIAL, &anMaterial);
    anariCommitParameters(anDevice, anSurface);
    anariRelease(anDevice, anMaterial);

    rLogger.debug("Created ANARI DEM surface");

    return std::shared_ptr<anari::api::Surface>(anSurface, [anDevice, pLogger = &rLogger](auto* anSurface) {
        anariRelease(anDevice, anSurface);
        pLogger->debug("Released ANARI DEM Surface");
    });
}