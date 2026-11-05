#pragma once

#include <im3e/property/property_interface.h>

#include <memory>
#include <string>

namespace im3e {

class IImguiPropertyControl
{
public:
    virtual ~IImguiPropertyControl() = default;

    /// @brief Draw the control, writing back to the property any edit made by the user.
    /// Does nothing once the property is destroyed.
    virtual void draw() = 0;

    virtual auto getName() const -> std::string = 0;
};

/// @brief Create the control matching the type of a property.
/// @throws std::invalid_argument if the property is expired.
auto createImguiPropertyControl(std::weak_ptr<IProperty> property) -> std::unique_ptr<IImguiPropertyControl>;

}  // namespace im3e
