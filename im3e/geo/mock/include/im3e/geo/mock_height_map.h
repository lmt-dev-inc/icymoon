#pragma once

#include <im3e/geo/height_map.h>

#include <gmock/gmock.h>

namespace im3e {

class MockHeightMapTileSampler : public IHeightMapTileSampler
{
public:
    MockHeightMapTileSampler();
    ~MockHeightMapTileSampler() override;

    MOCK_METHOD(float, at, (uint32_t x, uint32_t y), (const, override));
    MOCK_METHOD(float, at, (const glm::u32vec2& rPos), (const, override));

    MOCK_METHOD(bool, isValid, (uint32_t x, uint32_t y), (const, override));

    MOCK_METHOD(glm::u32vec2, getPos, (), (const, override));
    MOCK_METHOD(const glm::u32vec2&, getSize, (), (const, override));
    MOCK_METHOD(const glm::u32vec2&, getActualSize, (), (const, override));
    MOCK_METHOD(float, getScale, (), (const, override));
};

class MockHeightMap : public IHeightMap
{
public:
    MockHeightMap();
    ~MockHeightMap() override;

    MOCK_METHOD(void, rebuildPyramid, (), (override));

    MOCK_METHOD(std::unique_ptr<IHeightMapTileSampler>, getTileSampler, (const TileID& rTileID), (override));
    MOCK_METHOD(std::unique_ptr<IHeightMapTileSampler>, getTileSampler, (const glm::u32vec2& rTilePos, uint32_t lod),
                (override));

    MOCK_METHOD(std::string, getName, (), (const, override));
    MOCK_METHOD(glm::u32vec2, getSize, (), (const, override));
    MOCK_METHOD(glm::u32vec2, getTileSize, (), (const, override));
    MOCK_METHOD(glm::u32vec2, getTileCount, (uint32_t lod), (const, override));
    MOCK_METHOD(uint32_t, getLodCount, (), (const, override));
    MOCK_METHOD(float, getMinHeight, (), (const, override));
    MOCK_METHOD(float, getMaxHeight, (), (const, override));
};

}  // namespace im3e