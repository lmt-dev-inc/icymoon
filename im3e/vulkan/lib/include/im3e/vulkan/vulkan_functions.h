#pragma once

#include <im3e/utils/vk_utils.h>

namespace im3e {

/// @brief Pointers to Vulkan global functions.
/// The list of global functions can be found in vkGetInstanceProcAddr reference:
/// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#vkGetInstanceProcAddr
struct VulkanGlobalFcts
{
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion{};
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{};
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties{};
    PFN_vkCreateInstance vkCreateInstance{};
};

struct VulkanInstanceFcts
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{};
    PFN_vkDestroyInstance vkDestroyInstance{};
    PFN_vkCreateDevice vkCreateDevice{};
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR{};

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT{};
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{};

    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices{};
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{};
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures{};
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties{};
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties{};
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{};

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR{};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR{};
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR{};
};

struct VulkanDeviceFcts
{
    PFN_vkDestroyDevice vkDestroyDevice{};
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle{};
    PFN_vkGetDeviceQueue vkGetDeviceQueue{};
    PFN_vkQueueSubmit vkQueueSubmit{};
    PFN_vkQueueWaitIdle vkQueueWaitIdle{};
    PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout{};

    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{};

    PFN_vkCreateCommandPool vkCreateCommandPool{};
    PFN_vkDestroyCommandPool vkDestroyCommandPool{};
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{};
    PFN_vkFreeCommandBuffers vkFreeCommandBuffers{};
    PFN_vkResetCommandBuffer vkResetCommandBuffer{};
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer{};
    PFN_vkEndCommandBuffer vkEndCommandBuffer{};
    PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2{};
    PFN_vkCmdClearColorImage vkCmdClearColorImage{};
    PFN_vkCmdBlitImage vkCmdBlitImage{};
    PFN_vkCmdCopyImage vkCmdCopyImage{};
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass{};
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass{};

    PFN_vkCreateFence vkCreateFence{};
    PFN_vkDestroyFence vkDestroyFence{};
    PFN_vkWaitForFences vkWaitForFences{};
    PFN_vkResetFences vkResetFences{};

    PFN_vkCreateSemaphore vkCreateSemaphore{};
    PFN_vkDestroySemaphore vkDestroySemaphore{};

    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR{};
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR{};
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR{};
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR{};
    PFN_vkQueuePresentKHR vkQueuePresentKHR{};

    PFN_vkCreateFramebuffer vkCreateFramebuffer{};
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer{};
    PFN_vkCreateRenderPass vkCreateRenderPass{};
    PFN_vkDestroyRenderPass vkDestroyRenderPass{};
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool{};
    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool{};
    PFN_vkCreateImageView vkCreateImageView{};
    PFN_vkDestroyImageView vkDestroyImageView{};
    PFN_vkCreateSampler vkCreateSampler{};
    PFN_vkDestroySampler vkDestroySampler{};
};

}  // namespace im3e