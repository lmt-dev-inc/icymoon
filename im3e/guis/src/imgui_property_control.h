#pragma once

#include <im3e/property/property.h>

#include <memory>
#include <string>

namespace im3e {

class IImguiPropertyControl
{
public:
    virtual ~IImguiPropertyControl() = default;

    virtual void draw() = 0;

    virtual auto getName() const -> std::string = 0;
};

auto createImguiPropertyControl(std::shared_ptr<IProperty> pProperty) -> std::unique_ptr<IImguiPropertyControl>;

}  // namespace im3e