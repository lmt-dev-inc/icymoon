#include "geo.h"

#include <im3e/geo/mock_height_map.h>
#include <im3e/test_utils/glm.h>
#include <im3e/test_utils/test_utils.h>

#include <fmt/format.h>

using namespace im3e;

namespace {

void expectTreesEqual(HeightMapQuadTreeNode& rParent, HeightMapQuadTreeNode& rExpectedParent)
{
    const auto errorMessage = fmt::format("Tree Node at Tile ID ({}, {}, {})", rExpectedParent.tileID.x,
                                          rExpectedParent.tileID.y, rExpectedParent.tileID.z);

    EXPECT_THAT(rParent.tileID, Eq(rExpectedParent.tileID)) << errorMessage;
    EXPECT_THAT(rParent.minWorldPos, FloatEq(rExpectedParent.minWorldPos)) << errorMessage;
    EXPECT_THAT(rParent.maxWorldPos, FloatEq(rExpectedParent.maxWorldPos)) << errorMessage;
    for (auto i = 0U; i < 4U; i++)
    {
        const auto childErrorMessage = fmt::format("Child {} of {}", i, errorMessage);
        if (!rExpectedParent.pChildren[i])
        {
            EXPECT_THAT(rParent.pChildren[i], IsNull()) << childErrorMessage;
        }
        else
        {
            ASSERT_THAT(rParent.pChildren[i], NotNull()) << childErrorMessage;
            expectTreesEqual(*rParent.pChildren[i], *rExpectedParent.pChildren[i]);
        }
    }
}

}  // namespace

TEST(HeightMapQuadTreeTest, generateHeightMapQuadTree)
{
    constexpr glm::u32vec2 TestSize{520U, 1050U};
    constexpr glm::u32vec2 TestTileSize{512U, 512U};
    constexpr uint32_t TestLodCount{3U};
    constexpr auto TestMinHeight{-20.0F};
    constexpr auto TestMaxHeight{4809.0F};

    // Expected Level 2: a single tile
    auto pExpectedRoot = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
        .tileID = TileID{0U, 0U, 2U},
        .minWorldPos = glm::vec3{0.0F, TestMinHeight, 0.0F},
        .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 1050.0F},
    });

    // Expected Level 1: 2x1 tiles
    {
        pExpectedRoot->pChildren[0] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{0U, 0U, 1U},
            .minWorldPos = glm::vec3{0.0F, TestMinHeight, 0.0F},
            .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 1024.0F},
        });
        pExpectedRoot->pChildren[1] = nullptr;
        pExpectedRoot->pChildren[2] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{0U, 1U, 1U},
            .minWorldPos = glm::vec3{0.0F, TestMinHeight, 1024.0F},
            .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 1050.0F},
        });
        pExpectedRoot->pChildren[3] = nullptr;
    }

    // Expected Level 0: 2x3 tiles
    {
        auto pChild0L1 = pExpectedRoot->pChildren[0];
        pChild0L1->pChildren[0] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{0U, 0U, 0U},
            .minWorldPos = glm::vec3{0.0F, TestMinHeight, 0.0F},
            .maxWorldPos = glm::vec3{512.0F, TestMaxHeight, 512.0F},
        });
        pChild0L1->pChildren[1] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{1U, 0U, 0U},
            .minWorldPos = glm::vec3{512.0F, TestMinHeight, 0.0F},
            .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 512.0F},
        });
        pChild0L1->pChildren[2] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{0U, 1U, 0U},
            .minWorldPos = glm::vec3{0.0F, TestMinHeight, 512.0F},
            .maxWorldPos = glm::vec3{512.0F, TestMaxHeight, 1024.0F},
        });
        pChild0L1->pChildren[3] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{1U, 1U, 0U},
            .minWorldPos = glm::vec3{512.0F, TestMinHeight, 512.0F},
            .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 1024.0F},
        });
        auto pChild1L1 = pExpectedRoot->pChildren[2];
        pChild1L1->pChildren[0] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{0U, 2U, 0U},
            .minWorldPos = glm::vec3{0.0F, TestMinHeight, 1024.0F},
            .maxWorldPos = glm::vec3{512.0F, TestMaxHeight, 1050.0F},
        });
        pChild1L1->pChildren[1] = std::make_shared<HeightMapQuadTreeNode>(HeightMapQuadTreeNode{
            .tileID = TileID{1U, 2U, 0U},
            .minWorldPos = glm::vec3{512.0F, TestMinHeight, 1024.0F},
            .maxWorldPos = glm::vec3{520.0F, TestMaxHeight, 1050.0F},
        });
        pChild1L1->pChildren[2] = nullptr;
        pChild1L1->pChildren[3] = nullptr;
    }

    StrictMock<MockHeightMap> heightMap;
    EXPECT_CALL(heightMap, getSize()).WillRepeatedly(Return(TestSize));
    EXPECT_CALL(heightMap, getTileSize()).WillRepeatedly(Return(TestTileSize));
    EXPECT_CALL(heightMap, getLodCount()).WillRepeatedly(Return(TestLodCount));
    EXPECT_CALL(heightMap, getMinHeight()).WillRepeatedly(Return(TestMinHeight));
    EXPECT_CALL(heightMap, getMaxHeight()).WillRepeatedly(Return(TestMaxHeight));

    const auto pTreeRoot = generateHeightMapQuadTree(heightMap);
    ASSERT_THAT(pTreeRoot, NotNull());
    expectTreesEqual(*pTreeRoot, *pExpectedRoot);
}

TEST(HeightMapQuadTreeTest, findVisible)
{
    constexpr glm::u32vec2 TestSize{520U, 1050U};
    constexpr glm::u32vec2 TestTileSize{512U, 512U};
    constexpr auto TestMinHeight = 0.0F;
    constexpr auto TestMaxHeight = 10.0F;
    constexpr uint32_t TestLodCount{3U};

    StrictMock<MockHeightMap> heightMap;
    EXPECT_CALL(heightMap, getSize()).WillRepeatedly(Return(TestSize));
    EXPECT_CALL(heightMap, getTileSize()).WillRepeatedly(Return(TestTileSize));
    EXPECT_CALL(heightMap, getLodCount()).WillRepeatedly(Return(TestLodCount));
    EXPECT_CALL(heightMap, getMinHeight()).WillRepeatedly(Return(TestMinHeight));
    EXPECT_CALL(heightMap, getMaxHeight()).WillRepeatedly(Return(TestMaxHeight));

    const ViewFrustum::PerspectiveConfig viewConfig{
        .near = 0.1F,
        .far = 10000.0F,
        .position = glm::vec3{20.0F, 1000.0F, 10.0F},
        .direction = glm::vec3{0.0F, -1.0F, 0.0F},
        .up = glm::vec3{0.0F, 0.0F, -1.0F},
        .right = glm::vec3{1.0F, 0.0F, 0.0F},
    };

    const auto pTreeRoot = generateHeightMapQuadTree(heightMap);
    EXPECT_THAT(pTreeRoot->findVisible(viewConfig, 2U), ContainerEq(std::vector<TileID>{TileID{0U, 0U, 2U}}));
    EXPECT_THAT(pTreeRoot->findVisible(viewConfig, 0U), ContainerEq(std::vector<TileID>{
                                                            TileID{0U, 0U, 0U},
                                                            TileID{1U, 0U, 0U},
                                                            TileID{0U, 1U, 0U},
                                                            TileID{1U, 1U, 0U},
                                                        }));
}