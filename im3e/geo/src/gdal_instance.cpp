#include "gdal_instance.h"

#include <gdal.h>

#include <mutex>

using namespace im3e;

namespace {

class GdalInstance : public IGdalInstance
{
public:
    GdalInstance(const ILogger& rLogger)
      : m_pLogger(rLogger.createChild("GDAL"))
    {
        GDALAllRegister();
        m_pLogger->debug("Successfully initialized");
    }

    ~GdalInstance() override { m_pLogger->debug("Successfully deinitialized"); }

private:
    std::unique_ptr<ILogger> m_pLogger;
};

std::mutex g_mutex;
std::weak_ptr<GdalInstance> g_pInstance;

}  // namespace

auto im3e::getGdalInstance(const ILogger& rLogger) -> std::shared_ptr<IGdalInstance>
{
    std::lock_guard lock(g_mutex);

    if (auto pInstance = g_pInstance.lock())
    {
        return pInstance;
    }

    auto pInstance = std::make_shared<GdalInstance>(rLogger);
    g_pInstance = pInstance;
    return pInstance;
}