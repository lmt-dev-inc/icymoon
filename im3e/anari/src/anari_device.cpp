#include "anari_device.h"

#include "anari_frame_pipeline.h"
#include "anari_world.h"

#include <im3e/utils/core/throw_utils.h>
#include <im3e/utils/loggers.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <array>
#include <string_view>

using namespace im3e;

namespace {

auto chooseAnDeviceSubtype(const ILogger& rLogger, ANARILibrary anLib)
{
    auto pSubtypes = anariGetDeviceSubtypes(anLib);
    throwIfNull<std::runtime_error>(pSubtypes, "Failed to get device subtypes");

    std::vector<std::string> deviceSubtypes;
    for (const char** pSubtype = pSubtypes; *pSubtype != nullptr; pSubtype++)
    {
        deviceSubtypes.emplace_back(*pSubtype);
    }
    throwIfFalse<std::runtime_error>(!deviceSubtypes.empty(), "Failed to find ANARI device subtypes");
    rLogger.debug(fmt::format("Found the following device subtypes: {}", deviceSubtypes));
    rLogger.info(fmt::format("Selecting the first device subtype found: \"{}\"", deviceSubtypes.front()));
    return deviceSubtypes.front();
}

auto detectAnDeviceExtensions(const ILogger& rLogger, ANARILibrary pLib, std::string_view deviceSubtype)
{
    ANARIExtensions extensions{};
    if (anariGetDeviceExtensionStruct(&extensions, pLib, deviceSubtype.data()))
    {
        throw std::runtime_error("Failed to get ANARI device extensions");
    }

    auto checkExt = [&](int supported, std::string_view name) {
        rLogger.verbose(fmt::format("\t- {}: {}", name, supported ? "supported" : "not supported"));
    };
    checkExt(extensions.ANARI_KHR_CAMERA_PERSPECTIVE, "ANARI_KHR_CAMERA_PERSPECTIVE");
    checkExt(extensions.ANARI_KHR_GEOMETRY_TRIANGLE, "ANARI_KHR_GEOMETRY_TRIANGLE");
    checkExt(extensions.ANARI_KHR_LIGHT_DIRECTIONAL, "ANARI_KHR_LIGHT_DIRECTIONAL");
    checkExt(extensions.ANARI_KHR_LIGHT_POINT, "ANARI_KHR_LIGHT_POINT");
    checkExt(extensions.ANARI_KHR_MATERIAL_MATTE, "ANARI_KHR_MATERIAL_MATTE");
    checkExt(extensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED, "ANARI_KHR_MATERIAL_PHYSICALLY_BASED");

    return extensions;
}

auto createAnDevice(const ILogger& rLogger, ANARILibrary anLib, std::string_view deviceSubtype)
{
    auto anDevice = anariNewDevice(anLib, deviceSubtype.data());
    throwIfNull<std::runtime_error>(anDevice,
                                    fmt::format("Failed to create device with subtype \"{}\"", deviceSubtype));
    rLogger.info(fmt::format("Created device with subtype \"{}\"", deviceSubtype));

    anariCommitParameters(anDevice, anDevice);

    return UniquePtrWithDeleter<anari::api::Device>(anDevice, [pLogger = &rLogger, deviceSubtype](auto* anDevice) {
        anariRelease(anDevice, anDevice);
        pLogger->info(fmt::format("Destroyed device with subtype \"{}\"", deviceSubtype));
    });
}

auto toString(AnariPrimitiveType type)
{
    switch (type)
    {
        case AnariPrimitiveType::Triangle: return "triangle";

        default: break;
    }
    throw std::runtime_error{fmt::format("Unsupported ANARI primitive type: {}", static_cast<uint32_t>(type))};
}

auto toString(AnariMaterialType type)
{
    switch (type)
    {
        case AnariMaterialType::Matte: return "matte";
        case AnariMaterialType::PhysicallyBased: return "physicallyBased";
        default: break;
    }
    throw std::runtime_error{fmt::format("Unsupported ANARI material type: {}", static_cast<uint32_t>(type))};
}

}  // namespace

AnariDevice::AnariDevice(const ILogger& rLogger, ANARILibrary anLib, std::string_view anLibName,
                         ANARILibrary anDebugLib)
  : m_pLogger(rLogger.createChild("ANARI Device"))
  , m_anLibName(anLibName)
  , m_anDeviceSubtype(chooseAnDeviceSubtype(*m_pLogger, anLib))
  , m_anExtensions(detectAnDeviceExtensions(*m_pLogger, anLib, m_anDeviceSubtype))
  , m_pAnDebugWrappedDevice(anDebugLib ? createAnDevice(*m_pLogger, anLib, m_anDeviceSubtype) : nullptr)
  , m_pAnDevice(anDebugLib ? createAnDevice(*m_pLogger, anDebugLib, "debug")
                           : createAnDevice(*m_pLogger, anLib, m_anDeviceSubtype))
{
    if (m_pAnDebugWrappedDevice)
    {
        auto anWrappedDevice = m_pAnDebugWrappedDevice.get();
        anariSetParameter(m_pAnDevice.get(), m_pAnDevice.get(), "wrappedDevice", ANARI_DEVICE, &anWrappedDevice);
        anariCommitParameters(m_pAnDevice.get(), m_pAnDevice.get());
    }
}

auto AnariDevice::createArray1d(const void* pData, ANARIDataType type, size_t count)
    -> UniquePtrWithDeleter<anari::api::Array1D>
{
    auto anArray = anariNewArray1D(m_pAnDevice.get(), pData, nullptr, nullptr, type, count);
    return UniquePtrWithDeleter<anari::api::Array1D>(
        anArray, [anDevice = m_pAnDevice.get()](auto anArray) { anariRelease(anDevice, anArray); });
}

auto AnariDevice::createGroup(const std::vector<ANARISurface>& rAnSurfaces) -> UniquePtrWithDeleter<anari::api::Group>
{
    auto anGroup = anariNewGroup(m_pAnDevice.get());
    if (!rAnSurfaces.empty())
    {
        auto pAnSurfaceArray = createArray1d(rAnSurfaces, ANARI_SURFACE);
        auto anSurfaceArray = pAnSurfaceArray.get();
        anariSetParameter(m_pAnDevice.get(), anGroup, "surface", ANARI_ARRAY1D, &anSurfaceArray);
    }
    anariCommitParameters(m_pAnDevice.get(), anGroup);
    return UniquePtrWithDeleter<anari::api::Group>(
        anGroup, [anDevice = m_pAnDevice.get()](auto anGroup) { anariRelease(anDevice, anGroup); });
}

auto AnariDevice::createInstance(ANARIGroup anGroup) -> UniquePtrWithDeleter<anari::api::Instance>
{
    auto anInstance = anariNewInstance(m_pAnDevice.get(), "transform");
    if (anGroup)
    {
        anariSetParameter(m_pAnDevice.get(), anInstance, "group", ANARI_GROUP, &anGroup);
        anariCommitParameters(m_pAnDevice.get(), anInstance);
    }
    return UniquePtrWithDeleter<anari::api::Instance>(
        anInstance, [anDevice = m_pAnDevice.get()](auto anInstance) { anariRelease(anDevice, anInstance); });
}

auto AnariDevice::createGeometry(AnariPrimitiveType type) -> UniquePtrWithDeleter<anari::api::Geometry>
{
    auto anGeometry = anariNewGeometry(m_pAnDevice.get(), toString(type));
    return UniquePtrWithDeleter<anari::api::Geometry>{
        anGeometry, [this](auto anGeometry) { anariRelease(m_pAnDevice.get(), anGeometry); }};
}

auto AnariDevice::createMaterial(AnariMaterialType type) -> UniquePtrWithDeleter<anari::api::Material>
{
    auto anMaterial = anariNewMaterial(m_pAnDevice.get(), toString(type));
    return UniquePtrWithDeleter<anari::api::Material>{
        anMaterial, [this](auto anMaterial) { anariRelease(m_pAnDevice.get(), anMaterial); }};
}

auto AnariDevice::createSurface(ANARIGeometry anGeometry, ANARIMaterial anMaterial)
    -> UniquePtrWithDeleter<anari::api::Surface>
{
    throwIfArgNull(anGeometry, "Cannot create ANARI Surface without Geometry");
    throwIfArgNull(anMaterial, "Cannot create ANARI Surface without Material");

    auto anSurface = anariNewSurface(m_pAnDevice.get());

    anariSetParameter(m_pAnDevice.get(), anSurface, "geometry", ANARI_GEOMETRY, &anGeometry);
    anariSetParameter(m_pAnDevice.get(), anSurface, "material", ANARI_MATERIAL, &anMaterial);
    anariCommitParameters(m_pAnDevice.get(), anSurface);

    return UniquePtrWithDeleter<anari::api::Surface>{
        anSurface, [this](auto anSurface) { anariRelease(m_pAnDevice.get(), anSurface); }};
}

auto AnariDevice::createLogger(std::string_view name) -> std::unique_ptr<ILogger>
{
    return m_pLogger->createChild(name);
}