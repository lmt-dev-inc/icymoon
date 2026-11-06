#include "anari_renderer.h"

#include <im3e/property/property.h>

#include <fmt/format.h>

#include <array>
#include <stdexcept>

using namespace im3e;

namespace {

auto chooseAnRendererSubtype(const ILogger& rLogger, ANARIDevice pDevice)
{
    const auto** pSubtypes = anariGetObjectSubtypes(pDevice, ANARI_RENDERER);
    throwIfNull<std::runtime_error>(pSubtypes, "Failed to retrieve renderer subtypes");

    std::string subtypesMsg = "[";
    for (const auto** pSubtype = pSubtypes; *pSubtype != nullptr; pSubtype++)
    {
        if (pSubtype != pSubtypes)
        {
            subtypesMsg += ", ";
        }
        subtypesMsg += fmt::format("{}", *pSubtype);
    }
    subtypesMsg += "]";

    const std::string chosenSubtype = *pSubtypes;
    rLogger.info(fmt::format("Available renderer subtypes: {}. Choosing: {}", subtypesMsg, chosenSubtype));
    return chosenSubtype;
}

auto createAnRenderer(const ILogger& rLogger, ANARIDevice anDevice, std::string_view anSubtype)
{
    auto anRenderer = anariNewRenderer(anDevice, anSubtype.data());
    throwIfNull<std::runtime_error>(anRenderer, fmt::format("Failed to create renderer with subtype {}", anSubtype));

    constexpr std::array<float, 4U> BackgroundColor{0.3F, 0.3F, 0.4F, 1.0F};
    anariSetParameter(anDevice, anRenderer, "background", ANARI_FLOAT32_VEC4, BackgroundColor.data());

    anariCommitParameters(anDevice, anRenderer);
    rLogger.debug(fmt::format("Created renderer with subtype {}", anSubtype));

    return std::shared_ptr<anari::api::Renderer>(
        anRenderer, [anDevice, pLogger = &rLogger, anSubtype](auto* anRenderer) {
            anariRelease(anDevice, anRenderer);
            pLogger->debug(fmt::format("Destroyed renderer with subtype {}", anSubtype));
        });
}

template <typename T>
auto getRenderParamInfo(ANARIDevice anDevice, std::string_view rendererSubtype, const ANARIParameter* pAnParam,
                        std::string_view attribute, ANARIDataType attributeType)
{
    return static_cast<const T*>(anariGetParameterInfo(anDevice, ANARI_RENDERER, rendererSubtype.data(), pAnParam->name,
                                                       pAnParam->type, attribute.data(), attributeType));
}

struct PropertyFactory
{
    auto create(const ANARIParameter* anParam) -> std::unique_ptr<IPropertyWrapper>
    {
        switch (static_cast<int>(anParam->type))
        {
            case ANARI_BOOL: return _create<bool>(anParam);
            case ANARI_INT32: return _create<int32_t>(anParam);
            case ANARI_FLOAT32: return _create<float>(anParam);
            case ANARI_STRING: return _create<std::string>(anParam);
            case ANARI_FLOAT32_VEC3: return _create<glm::vec3>(anParam);
            case ANARI_FLOAT32_VEC4: return _create<glm::vec4>(anParam);
            case ANARI_ARRAY2D: return _create<void*>(anParam);
            case ANARI_UNKNOWN: throw std::runtime_error(fmt::format("Unsupported UNKNOWN ANARI data type"));
            default: break;
        }
        throw std::runtime_error(fmt::format("Unsupported ANARI data type: {}", static_cast<int>(anParam->type)));
    }

    ANARIDevice anDevice;
    ANARIRenderer anRenderer;
    std::string_view anRendererSubtype;

private:
    template <typename T>
    auto _create(const ANARIParameter* anParam) -> std::unique_ptr<IPropertyWrapper>
    {
        constexpr auto IsString = std::is_same_v<T, std::string>;
        using ParamType = std::conditional_t<IsString, char, T>;

        const auto* description = getRenderParamInfo<char>(anDevice, anRendererSubtype, anParam, "description",
                                                           ANARI_STRING);
        const auto* defaultValue = getRenderParamInfo<ParamType>(anDevice, anRendererSubtype, anParam, "default",
                                                                 anParam->type);

        typename Property<T>::Config config{
            .name = anParam->name,
            .description = description ? description : "",
        };
        if (defaultValue)
        {
            if constexpr (IsString)
            {
                config.defaultValue = defaultValue;
            }
            else
            {
                config.defaultValue = *defaultValue;
            }
        }
        return std::make_unique<Property<T>>(
            std::move(config), [anDevice = anDevice, anRenderer = anRenderer, anParam](const T& value) {
                if constexpr (IsString)
                {
                    anariSetParameter(anDevice, anRenderer, anParam->name, anParam->type, value.c_str());
                }
                else
                {
                    anariSetParameter(anDevice, anRenderer, anParam->name, anParam->type, &value);
                }
            });
    }
};

auto createProperties(const AnariDevice& rAnDevice, ANARIRenderer anRenderer, std::string_view anRendererSubtype)
    -> std::vector<std::unique_ptr<IPropertyWrapper>>
{
    const auto* anParams = static_cast<const ANARIParameter*>(anariGetObjectInfo(
        rAnDevice.getHandle(), ANARI_RENDERER, anRendererSubtype.data(), "parameter", ANARI_PARAMETER_LIST));

    PropertyFactory propertyFactory{
        .anDevice = rAnDevice.getHandle(),
        .anRenderer = anRenderer,
        .anRendererSubtype = anRendererSubtype,
    };
    std::vector<std::unique_ptr<IPropertyWrapper>> properties;
    for (const auto* anParam = anParams; anParam->name != nullptr; anParam++)
    {
        properties.emplace_back(propertyFactory.create(anParam));
    }
    return properties;
}

auto getInterfaces(const std::vector<std::unique_ptr<IPropertyWrapper>>& properties)
    -> std::vector<std::weak_ptr<IPropertyNode>>
{
    std::vector<std::weak_ptr<IPropertyNode>> interfaces;
    interfaces.reserve(properties.size());
    for (const auto& property : properties)
    {
        interfaces.emplace_back(property->getInterface());
    }
    return interfaces;
}

}  // namespace

AnariRenderer::AnariRenderer(std::shared_ptr<AnariDevice> pAnDevice)
  : m_pAnDevice(throwIfArgNull(std::move(pAnDevice), "ANARI Renderer requires an ANARI device"))
  , m_pLogger(m_pAnDevice->createLogger("ANARI Renderer"))
  , m_anRendererSubtype(chooseAnRendererSubtype(*m_pLogger, m_pAnDevice->getHandle()))
  , m_pAnRenderer(createAnRenderer(*m_pLogger, m_pAnDevice->getHandle(), m_anRendererSubtype))
  , m_properties(createProperties(*m_pAnDevice, m_pAnRenderer.get(), m_anRendererSubtype))
  , m_propertyGroup{
        {
            .name = fmt::format("Renderer: \"{}\"", m_anRendererSubtype),
            .description = "Parameters of the ANARI renderer",
        },
        getInterfaces(m_properties),
        [this] { m_parametersChanged = true; },
    }
{
}

void AnariRenderer::commitChanges()
{
    if (m_parametersChanged)
    {
        anariCommitParameters(m_pAnDevice->getHandle(), m_pAnRenderer.get());
        m_parametersChanged = false;
    }
}
