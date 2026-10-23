#pragma once

#include "command_buffer.h"

#include <im3e/utils/core/flags.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace im3e {

class IGuiPanel
{
public:
    virtual ~IGuiPanel() = default;

    virtual void draw(const ICommandBuffer& rCommandBuffer) = 0;

    /// @brief Called whenever the window this panel belongs to is resized.
    /// @param[in] rWindowSize New window size
    /// @param[in] vkFormat Format of the window
    /// @param[in] frameInFlightCount Number of frames that can be in flight for rendering at a time.
    virtual void onWindowResized([[maybe_unused]] const VkExtent2D& rWindowSize, [[maybe_unused]] VkFormat vkFormat,
                                 [[maybe_unused]] uint32_t frameInFlightCount)
    {
    }

    virtual auto getName() const -> std::string = 0;
};

class IGuiWorkspace : public IGuiPanel
{
public:
    virtual ~IGuiWorkspace() = default;

    enum class Location
    {
        Top,
        Bottom,
        Center,
        Left,
        Right
    };
    virtual void addPanel(Location location, std::shared_ptr<IGuiPanel> pPanel, float fraction = 0.25F) = 0;
};

enum class MouseButton : uint32_t
{
    Left = 1 << 0U,
    Right = 1U << 1U,
    Middle = 1U << 2U,
};
template <>
inline constexpr bool IsFlagEnum<MouseButton> = true;
using MouseButtons = Flags<MouseButton>;

enum class KeyModifier : uint32_t
{
    Shift = 1U << 0U,
    Ctrl = 1U << 1U,
    Alt = 1U << 2U,
    Super = 1U << 3U,
};
template <>
inline constexpr bool IsFlagEnum<KeyModifier> = true;
using KeyModifiers = Flags<KeyModifier>;

class IGuiEventListener
{
public:
    virtual ~IGuiEventListener() = default;

    virtual void onMouseMove(const glm::vec2& rClipOffset, MouseButtons mouseButtons, KeyModifiers keyModifiers) = 0;
    virtual void onMouseWheel(float scrollSteps) = 0;
};

}  // namespace im3e