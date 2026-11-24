# Vulkan

This document describes `im3e_vulkan`, the library that holds the Vulkan code of the project: the contracts other libraries use to work with the GPU, how they are implemented, and how they are tested. It ends with the known limitations, as input for the planned Vulkan renderer.

- [Vulkan](#vulkan)
  - [Overview](#overview)
  - [Public headers](#public-headers)
    - [Interfaces and helper functions](#interfaces-and-helper-functions)
  - [Device creation](#device-creation)
  - [Function loading](#function-loading)
  - [Memory and images](#memory-and-images)
  - [Commands and synchronization](#commands-and-synchronization)
  - [Frame pipelines](#frame-pipelines)
  - [Tests](#tests)
  - [Known limitations](#known-limitations)

## Overview

[im3e/vulkan/](../../im3e/vulkan/) is the GPU layer of the project. It is the only library that creates Vulkan objects behind interfaces; the others use these interfaces:

- `im3e_guis` presents to windows (`Presenter`, swapchains) and draws ImGui with its Vulkan back-end.
- `im3e_anari` copies the frames rendered by ANARI into Vulkan images.
- `im3e_render_vulkan`, the native renderer that will replace ANARI, renders with the device (see [Vulkan renderer](index.md#vulkan-renderer-im3e_render_vulkan)).
- `im3e_api` declares the GUI contracts, which take an `ICommandBuffer` (`IGuiPanel::draw`) and return an `IDevice` (`IWindowApplication::getDevice`).
- `im3e_test_utils` creates a device for integration tests.

The libraries below it (`im3e_utils_core`, `im3e_utils`, `im3e_property`, `im3e_scene`, `im3e_geo`) don't depend on Vulkan. `im3e_vulkan` publicly links the Vulkan headers, VMA (Vulkan Memory Allocator) and `im3e_utils`.

It follows the layout of [Library folders](../development/code_standards.md#library-folders):

```
im3e/vulkan/                group
  lib/                      im3e_vulkan
    include/im3e/vulkan/    public headers
    src/                    implementation, private interfaces (IVulkanLoader, IVulkanMemoryAllocator)
  mock/                     mock_im3e_vulkan: mocks of the public interfaces, for the tests of any library
  test/
    common/                 test_common_im3e_vulkan: mocks of the private interfaces, for the tests of im3e_vulkan
    unit/                   test_im3e_vulkan
    integration/            integration_im3e_vulkan
```

## Public headers

Public headers are included through `<im3e/vulkan/...>`:

| Header               | Content                                                                                                                                                                                                 |
| -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `device.h`           | `IDevice`: Vulkan handles and function tables, image factory, command queue, logger and stats factories. `createDevice(logger, DeviceConfig)` creates the Vulkan device.                                |
| `image.h`            | `IImage`, `IHostVisibleImage` (CPU mapping), `IImageView`, `IImageMetadata` (current layout and access, for automatic barriers), `IImageFactory`.                                                       |
| `command_buffer.h`   | `ICommandQueue::startScopedCommand` returns a command buffer that is submitted when it goes out of scope, sync or async. `ICommandBuffer` records scoped image barriers and creates futures to wait on. |
| `frame_pipeline.h`   | `IFramePipeline`: records the commands that render a frame into an output image, and handles resizes.                                                                                                   |
| `vulkan_functions.h` | `VulkanGlobalFcts`, `VulkanInstanceFcts`, `VulkanDeviceFcts`: tables of the Vulkan functions the project uses.                                                                                          |
| `utils.h`            | Helpers rather than interfaces: `VkUniquePtr`/`VkSharedPtr` for Vulkan handles, `throwIfVkFailed`, `getVkList`, `getFormatProperties`, `VkExtent2D` comparisons.                                        |
| `sync_utils.h`       | `createVkSemaphore`, `createVkFence` and `waitForVkFence`, built on `IDevice`.                                                                                                                          |

### Interfaces and helper functions

An interface only declares the functions that need the state of its implementation. A function that can be written with the public functions of an interface is a free function taking the interface, rather than a member of it.

For example, `createVkSemaphore(device)`, `createVkFence(device, flags)` and `waitForVkFence(device, fence)` only use `IDevice::getVkDevice()` and `getFcts()`, so they are free functions of `sync_utils.h`. `createLogger`, `getImageFactory` and `getCommandQueue` return objects the device owns or configures, so they stay members of `IDevice`.

This keeps the interfaces and their mocks small, and keeps `device.h` from including the headers of every helper, such as `im3e_shader` for shader modules. Tests check the helpers through the Vulkan calls they make, with `MockVulkanFunctions`, as for any other Vulkan code.

The headers form three levels, so that each one includes the headers it needs without cycles:

1. `utils.h` holds what the interfaces themselves use, such as `VkUniquePtr` and `throwIfVkFailed`. It doesn't depend on any interface of `im3e_vulkan`, as `device.h`, `image.h` and `command_buffer.h` include it.
2. The interface headers: `device.h`, `image.h`, `command_buffer.h`, `frame_pipeline.h`.
3. Helpers built on the interfaces, in headers named after what they cover, with a `_utils` suffix: `sync_utils.h` for semaphores and fences, and later a `shader_utils.h` for shader modules and pipelines, which needs `im3e_shader`. They include the interface headers they use. A header named after the interface, such as `device_utils.h`, is avoided, as most helpers take an `IDevice` and it would collect unrelated functions.

Helpers are implemented in `.cpp` files rather than inline: they call Vulkan through function pointers, which costs far more than the call itself, and they are not called in hot loops. Only trivial functions, such as `toVkExtent3D`, and templates are defined in headers.

## Device creation

`createDevice(logger, DeviceConfig)` creates the whole Vulkan stack. `DeviceConfig` carries the debug flag, a callback to check presentation support (provided by GLFW) and the instance extensions the window system needs. Applications currently get the device from `GlfwWindowApplication`, which calls `createDevice` itself.

- `VulkanInstance` creates a Vulkan 1.3 instance. When debug is enabled, it adds the `VK_LAYER_KHRONOS_validation` layer and a `VulkanDebugMessageHandler`, which forwards validation messages to the logger at their severity. Validation errors are therefore logged as errors, which make integration tests fail.
- `VulkanPhysicalDevices` ranks the physical devices. A device needs Vulkan 1.3, all the required extensions, a graphics queue and a transfer queue, and a presentation queue when a presentation check is given. Discrete GPUs come first, then integrated GPUs, then other types, such as CPU implementations.
- `VulkanDevice` creates the logical device with the only required extension, swapchain. It enables Vulkan 1.1 to 1.3 features, among which `synchronization2`, `dynamicRendering`, `bufferDeviceAddress`, descriptor indexing and `scalarBlockLayout`. New pipelines use dynamic rendering (`vkCmdBeginRendering`) rather than `VkRenderPass` objects, which only the ImGui back-end still uses. It owns a single `VulkanCommandQueue`, the memory allocator and the image factory.

## Function loading

The whole project is compiled with `VK_NO_PROTOTYPES`: no code calls Vulkan through symbols exported by the Vulkan library. Every call goes through the function tables of `vulkan_functions.h`, returned by `IDevice::getFcts()` and `getInstanceFcts()`.

`VulkanLoader`, behind the private `IVulkanLoader` interface ([vulkan_loader.h](../../im3e/vulkan/lib/src/vulkan_loader.h)), loads `vulkan-1.dll` or `libvulkan.so.1` at run time and fills the function tables, plus the table VMA needs. `createVulkanLoader(config)` opens the system library; an overload taking the entry points lets unit tests replace them with fakes.

This keeps Vulkan an optional run-time dependency, and makes every Vulkan call mockable in unit tests (`MockVulkanFunctions`).

## Memory and images

- **VMA** allocates memory, behind the private `IVulkanMemoryAllocator` interface. Its functions are also loaded through the loader (`VMA_STATIC_VULKAN_FUNCTIONS=0` and `VMA_DYNAMIC_VULKAN_FUNCTIONS=0` in the root `CMakeLists.txt`). Its implementation is compiled once, in `vulkan_memory_allocator.cpp`.
- **`IImageFactory`** creates three kinds of images:
  - device-local images: optimal tiling, in GPU memory;
  - host-visible images: linear tiling, mappable by the CPU (`map`, `mapReadOnly`), and savable to a file, e.g. when a pipeline integration test fails;
  - proxy images, for images the device does not own, such as swapchain images.
- **`IImageMetadata`** tracks the current layout, last pipeline stage and last access of each image. Barriers therefore only need the destination state: the source state comes from the metadata, which the barrier then updates.

## Commands and synchronization

- `ICommandQueue::startScopedCommand(name, executionType)` returns a command buffer, submitted when it goes out of scope. With `CommandExecutionType::Sync`, the submission waits for the commands to complete; with `Async`, it returns immediately.
- `ICommandBuffer::startScopedBarrier(name)` returns a recorder: images added with `addImageBarrier(image, {stage, access, layout})` get their barriers recorded together when the recorder goes out of scope, with `synchronization2`.
- `ICommandBuffer::createFuture()` returns an `ICommandBufferFuture`, backed by the fence of the submission, to wait for asynchronous commands.
- `setVkWaitSemaphore` and `setVkSignalSemaphore` synchronize a submission with the swapchain, for presentation.

## Frame pipelines

`IFramePipeline` is the central abstraction for rendering: `prepareExecution(commandBuffer, viewportSize, outputImage)` records the commands that render a frame into an output image, and `resize(extent, frameInFlightCount)` adapts to a new size. The ImGui GUI (`ImguiPipeline`), the ANARI renderer (`AnariFramePipeline`), the Vulkan renderer (`VulkanFramePipeline`, not implemented yet) and the test pipelines implement it, and they can be nested: the ImGui pipeline draws a render panel, which runs an ANARI pipeline into an offscreen image. See [Run-time flow of a frame](index.md#run-time-flow-of-a-frame).

## Tests

- **Unit tests** (`test_im3e_vulkan`) run without a GPU: `MockVulkanLoader` returns `MockVulkanFunctions`, so every Vulkan call can be checked with GoogleMock. `test_common_im3e_vulkan` holds the mocks of the private interfaces (`MockVulkanLoader`, `MockVulkanMemoryAllocator`) and helpers that set up common expectations. The tests of other libraries use the mocks of the public interfaces from `mock_im3e_vulkan` (`MockDevice`, `MockImage`, `MockCommandBuffer`, `MockVulkanFunctions`).
- **Integration tests** (`integration_im3e_vulkan`) need a Vulkan driver and the validation layers. They use the fixtures of `im3e_test_utils`: `DeviceIntegrationTest` creates a device with debug enabled, and `PipelineIntegrationTest` runs an `IFramePipeline` into an image and compares its pixels. `<im3e/test_utils/vk.h>` has GoogleTest printers for Vulkan types.

## Known limitations

- **`IDevice` exposes raw Vulkan handles and function tables.** Users call Vulkan directly, so the interfaces don't hide Vulkan from them.
- **Single queue.** `IDevice` has one command queue, used for graphics, transfers and presentation.
- **The window application creates the device.** A host application can't provide its own device, nor create one without GLFW (see `GlfwWindowApplication`).
- **No rendering resources yet.** A Vulkan renderer needs buffers, shader modules, graphics pipelines, descriptor sets and depth images, which `im3e_vulkan` doesn't provide yet. Shaders are already compiled to SPIR-V at build time, by `im3e_shader` (see [Shaders: `im3e_shader`](index.md#shaders-im3e_shader)).
