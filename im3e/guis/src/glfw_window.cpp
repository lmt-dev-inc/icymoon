#include "glfw_window.h"

#include "guis.h"
#include "imgui_pipeline.h"

#include <fmt/format.h>

using namespace im3e;

namespace {

GlfwWindowCallbacks& getWindowCallbacks(GLFWwindow* pGlfwWindow)
{
    return *reinterpret_cast<GlfwWindowCallbacks*>(glfwGetWindowUserPointer(pGlfwWindow));
}

void framebufferResizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
{
    getWindowCallbacks(pGlfwWindow).onWindowResized(width, height);
}

void windowIconifyCallback(GLFWwindow* pGlfwWindow, int iconified)
{
    getWindowCallbacks(pGlfwWindow).onWindowIconify(iconified == GLFW_TRUE);
}

auto createWindow(const ILogger& rLogger, const GlfwWindow::Config& rConfig, GlfwWindowCallbacks* pCallbacks)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // for Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, rConfig.maximized ? GLFW_TRUE : GLFW_FALSE);

    constexpr int Width = 1200;
    constexpr int Height = 800;

    auto* pGlfwWindow = glfwCreateWindow(Width, Height, rConfig.name.data(), nullptr, nullptr);
    throwIfNull<std::runtime_error>(pGlfwWindow, "Could not create GLFW window");

    glfwSetWindowUserPointer(pGlfwWindow, pCallbacks);
    glfwSetFramebufferSizeCallback(pGlfwWindow, framebufferResizeCallback);
    glfwSetWindowIconifyCallback(pGlfwWindow, windowIconifyCallback);

    int width{}, height{};
    glfwGetWindowSize(pGlfwWindow, &width, &height);
    rLogger.info(fmt::format("Created window of size {}x{}", width, height));

    // Setting the GLFW_MAXIMIMZED flag above is not always enough. Sometimes, when the window is open on a different
    // screen than the primary one, the window does not show maximized untile glfwMaximizeWindow() is called.
    glfwShowWindow(pGlfwWindow);
    if (rConfig.maximized)
    {
        glfwMaximizeWindow(pGlfwWindow);
    }

    return UniquePtrWithDeleter<GLFWwindow>(pGlfwWindow, [](auto* pW) { glfwDestroyWindow(pW); });
}

auto createVkSurface(const IDevice& rDevice, GLFWwindow* pWindow)
{
    const auto vkInstance = rDevice.getVkInstance();

    VkSurfaceKHR surface{};
    throwIfVkFailed(glfwCreateWindowSurface(vkInstance, pWindow, nullptr, &surface), "Could not create window surface");

    return VkUniquePtr<VkSurfaceKHR>(surface, [fcts = rDevice.getInstanceFcts(), vkInstance](auto* s) {
        fcts.vkDestroySurfaceKHR(vkInstance, s, nullptr);
    });
}

}  // namespace

GlfwWindow::GlfwWindow(std::shared_ptr<IDevice> pDevice, Config config, std::shared_ptr<ImguiWorkspace> pWorkspace)
  : m_pDevice(throwIfArgNull(std::move(pDevice), "Glfw window requires a device"))
  , m_config(std::move(config))
  , m_pWorkspace(throwIfArgNull(std::move(pWorkspace), "Glfw window requires a workspace"))
  , m_pLogger(m_pDevice->createLogger(m_config.name))
  , m_pCallbacks([&] {
      auto pCallbacks = std::make_unique<GlfwWindowCallbacks>();
      pCallbacks->onWindowResized = [this](auto w, auto h) { this->_onWindowResized(w, h); };
      pCallbacks->onWindowIconify = [this](bool i) { this->_onWindowIconify(i); };
      return pCallbacks;
  }())
  , m_pWindow(createWindow(*m_pLogger, m_config, m_pCallbacks.get()))
  , m_pVkSurface(createVkSurface(*m_pDevice, m_pWindow.get()))
  , m_pPresenter(std::make_unique<Presenter>(
        m_pDevice, m_pVkSurface.get(),
        std::make_unique<ImguiPipeline>(m_pDevice, m_pWindow.get(), m_pWorkspace, m_config.iniFilename)))
{
}

void GlfwWindow::draw()
{
    m_pPresenter->present();
}

void GlfwWindow::_onWindowResized(int width, int height)
{
    m_pPresenter->reset();
    m_pLogger->info(fmt::format("Resized to {}x{}", width, height));
}

void GlfwWindow::_onWindowIconify(bool iconify)
{
    m_iconified = iconify;
    m_pLogger->info(fmt::format("Window iconify set to {}", iconify));
}