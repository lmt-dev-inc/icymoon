#pragma once

#include "anari.h"
#include "anari_device.h"
#include "anari_height_field_tile.h"
#include "anari_instance_set.h"
#include "anari_map_camera.h"

#include <im3e/api/height_map.h>
#include <im3e/geo/geo.h>
#include <im3e/property/property.h>
#include <im3e/property/property_group.h>
#include <im3e/utils/core/types.h>
#include <im3e/utils/math_utils.h>

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace im3e {

class AnariHeightField : public IAnariObject
{
public:
    AnariHeightField(std::shared_ptr<AnariDevice> pAnDevice, AnariInstanceSet& rInstanceSet,
                     std::unique_ptr<IHeightMap> pHeightMap);

    void updateAsync(const AnariMapCamera& rCamera);
    void commitChanges();

    auto getProperties() -> std::weak_ptr<IPropertyGroup> override { return m_properties.getInterface(); }

private:
    std::shared_ptr<AnariDevice> m_pAnDevice;
    AnariInstanceSet& m_rInstanceSet;
    std::unique_ptr<IHeightMap> m_pHeightMap;

    std::unique_ptr<ILogger> m_pLogger;
    std::shared_ptr<HeightMapQuadTreeNode> m_pQuadTreeRoot;

    bool m_lodChanged = true;
    Property<uint32_t> m_lod;
    PropertyGroup m_properties;

    std::vector<std::unique_ptr<AnariHeightFieldTile>> m_pTiles;
    std::vector<UniquePtrWithDeleter<AnariHeightFieldTile>> m_pVisibleTiles;
    std::deque<AnariHeightFieldTile*> m_pAvailableTilesQueue;
};

}  // namespace im3e