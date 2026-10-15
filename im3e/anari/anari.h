#pragma once

#include <im3e/api/device.h>
#include <im3e/api/frame_pipeline.h>
#include <im3e/api/gui.h>
#include <im3e/api/height_map.h>
#include <im3e/utils/loggers.h>
#include <im3e/utils/properties/properties.h>

#include <anari/anari.h>

#include <memory>
#include <string_view>

namespace im3e {

class IAnariObject
{
public:
    virtual ~IAnariObject() = default;

    virtual auto getProperties() -> std::shared_ptr<IPropertyGroup> = 0;
};

class IAnariWorld
{
public:
    virtual ~IAnariWorld() = default;

    virtual auto addPlane(std::string_view name) -> std::shared_ptr<IAnariObject> = 0;
    virtual auto addHeightField(std::unique_ptr<IHeightMap> pHeightMap) -> std::shared_ptr<IAnariObject> = 0;
};

class IAnariFramePipeline : public IFramePipeline
{
public:
    virtual ~IAnariFramePipeline() = default;

    virtual auto createRendererProperties() -> std::shared_ptr<IPropertyGroup> = 0;
    virtual auto getCameraListener() -> std::shared_ptr<IGuiEventListener> = 0;
    virtual auto getWorld() -> std::shared_ptr<IAnariWorld> = 0;
};

class IAnariEngine
{
public:
    virtual ~IAnariEngine() = default;

    virtual auto createFramePipeline() -> std::unique_ptr<IAnariFramePipeline> = 0;
};

auto createAnariEngine(const ILogger& rLogger, std::shared_ptr<IDevice> pDevice, bool debugEnabled = false)
    -> std::unique_ptr<IAnariEngine>;

}  // namespace im3e