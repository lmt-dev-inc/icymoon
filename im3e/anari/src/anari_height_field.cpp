#include "anari_height_field.h"

#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>

using namespace im3e;

namespace {

auto initializeTiles(std::shared_ptr<AnariDevice> pAnDevice, const glm::u32vec2& rTileSize, size_t count)
{
    std::vector<std::unique_ptr<AnariHeightFieldTile>> pTiles(count);
    std::ranges::generate(pTiles, [&] { return std::make_unique<AnariHeightFieldTile>(pAnDevice, rTileSize); });
    return pTiles;
}

}  // namespace

AnariHeightField::AnariHeightField(std::shared_ptr<AnariDevice> pAnDevice, AnariInstanceSet& rInstanceSet,
                                   std::unique_ptr<IHeightMap> pHeightMap)
  : m_pAnDevice(throwIfArgNull(std::move(pAnDevice), "ANARI Height Field requires a device"))
  , m_rInstanceSet(rInstanceSet)
  , m_pHeightMap(throwIfArgNull(std::move(pHeightMap), "ANARI Height Map requires a height map"))
  , m_pLogger(m_pAnDevice->createLogger(fmt::format("ANARI Height Field - {}", m_pHeightMap->getName())))
  , m_pQuadTreeRoot(generateHeightMapQuadTree(*m_pHeightMap))

  , m_pLodProp(std::make_shared<PropertyValue<uint32_t>>(PropertyValueConfig<uint32_t>{
        .name = "Level of Details",
        .description = "Determines level of details of the height field where 0 is highest details.",
        .defaultValue = 5U,
        .minValue = 0U,
        .maxValue = m_pHeightMap->getLodCount() - 1U,
        .onChange = [this](auto) { m_lodChanged = true; },
    }))
  , m_pProperties(createPropertyGroup(m_pHeightMap->getName(), {m_pLodProp}))

  , m_pTiles(initializeTiles(m_pAnDevice, m_pHeightMap->getTileSize(), 100U))
  , m_pAvailableTilesQueue([this] {
      std::deque<AnariHeightFieldTile*> pAvailableTilesQueue;
      for (auto& rpTile : m_pTiles)
      {
          pAvailableTilesQueue.emplace_back(rpTile.get());
      }
      return pAvailableTilesQueue;
  }())
{
}

void AnariHeightField::updateAsync([[maybe_unused]] const AnariMapCamera& rCamera)
{
    // TODO: frustum culling not fully working.
    // When zoomed out and switching between levels of details, some tiles that should be visible are not
    auto visibleTileIDs = m_pQuadTreeRoot->findVisible(rCamera.getViewFrustum(), m_pLodProp->getValue());
    // m_pLogger->debug(fmt::format("Found {} visible tiles", visibleTileIDs.size()));

    // Removed no longer visible tiles
    auto visibleTileIt = m_pVisibleTiles.begin();
    while (visibleTileIt != m_pVisibleTiles.end())
    {
        auto& rpVisibleTile = *visibleTileIt;
        if (auto itFind = std::ranges::find(visibleTileIDs, rpVisibleTile->getTileID()); itFind != visibleTileIDs.end())
        {
            visibleTileIDs.erase(itFind);
            visibleTileIt++;
            continue;
        }
        visibleTileIt = m_pVisibleTiles.erase(visibleTileIt);
    }

    auto useAvailableTile = [&](AnariHeightFieldTile* pTile) {
        m_rInstanceSet.insert(pTile->getInstance());
        m_pVisibleTiles.emplace_back(
            UniquePtrWithDeleter<AnariHeightFieldTile>(pTile, [this](AnariHeightFieldTile* pTile) {
                m_rInstanceSet.remove(pTile->getInstance());
                m_pAvailableTilesQueue.emplace_back(pTile);
            }));
    };

    // Insert visible tiles that are not loaded yet:
    for (auto& rVisibleTileID : visibleTileIDs)
    {
        // See if any available tile already has the data. If so, let's reuse it instead of reloading the data:
        if (auto itFind = std::ranges::find_if(
                m_pAvailableTilesQueue,
                [&](auto& rpAvailableTile) { return rpAvailableTile->getTileID() == rVisibleTileID; });
            itFind != m_pAvailableTilesQueue.end())
        {
            auto pTile = *itFind;
            m_pAvailableTilesQueue.erase(itFind);
            useAvailableTile(pTile);
            continue;
        }

        // If we reach this point, the current tile must be loaded from any available tile or skip the tile if we no
        // longer have any available tile:
        if (m_pAvailableTilesQueue.empty())
        {
            continue;
        }
        auto pAvailableTile = m_pAvailableTilesQueue.front();
        if (pAvailableTile->load(*m_pHeightMap->getTileSampler(rVisibleTileID)))
        {
            m_pAvailableTilesQueue.pop_front();
            useAvailableTile(pAvailableTile);
        }
    }
}

void AnariHeightField::commitChanges()
{
    std::ranges::for_each(m_pTiles, [](auto& rpTile) { rpTile->commitChanges(); });
}
