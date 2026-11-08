#pragma once

#include "glfw_window.h"
#include "guis.h"

#include <im3e/api/window.h>
#include <im3e/vulkan/device.h>

#include <memory>

namespace im3e {

class GlfwInstance
{
public:
    GlfwInstance(const ILogger& rLogger);
    ~GlfwInstance();

    auto getRequiredExtensions() const -> std::vector<const char*>;

private:
    const ILogger& m_rLogger;
};

class GlfwWindowApplication : public IWindowApplication
{
public:
    GlfwWindowApplication(const ILogger& rLogger, WindowApplicationConfig config);

    void createWindow(WindowConfig config, std::shared_ptr<IGuiWorkspace> pWorkspace) override;

    void run(std::function<void()> loopIterationFct) override;
    void stop() override;

    auto getDevice() -> std::shared_ptr<IDevice> override { return m_pDevice; }
    auto getDevice() const -> std::shared_ptr<const IDevice> override { return m_pDevice; }

private:
    const WindowApplicationConfig m_config;
    std::unique_ptr<ILogger> m_pLogger;

    std::unique_ptr<GlfwInstance> m_pGlfwInstance;
    std::shared_ptr<IDevice> m_pDevice;

    std::vector<std::unique_ptr<GlfwWindow>> m_pWindows;
};

}  // namespace im3e