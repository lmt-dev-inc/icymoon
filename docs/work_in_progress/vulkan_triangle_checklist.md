# Vulkan Triangle Checklist

Progress on the Vulkan objects needed to draw the triangle of milestone 2 of the [Vulkan Renderer Plan](vulkan_renderer_plan.md#2-triangle). The triangle's vertices are hard-coded in its vertex shader, so there is no vertex input.

- [Vulkan Triangle Checklist](#vulkan-triangle-checklist)
  - [Device and loader](#device-and-loader)
  - [Shader](#shader)
  - [Created once](#created-once)
  - [Created on resize](#created-on-resize)
  - [Recorded each frame](#recorded-each-frame)
  - [Not needed yet](#not-needed-yet)

## Device and loader

- [x] Enable `dynamicRendering` in `VkPhysicalDeviceVulkan13Features` (check that `synchronization2` is enabled too).
- [x] Add the missing functions to the loader tables and `MockVulkanFunctions`:
  - [x] `vkCreateShaderModule`, `vkDestroyShaderModule`
  - [x] `vkCreatePipelineLayout`, `vkDestroyPipelineLayout`
  - [x] `vkCreateGraphicsPipelines`, `vkDestroyPipeline`
  - [x] `vkCmdBeginRendering`, `vkCmdEndRendering`
  - [x] `vkCmdBindPipeline`, `vkCmdSetViewport`, `vkCmdSetScissor`, `vkCmdDraw`
  - [x] `vkCmdBlitImage`: already loaded and mocked

## Shader

- [x] Vertex shader: a `static const float2[3]` indexed by `SV_VertexID`, returning `float4(pos, 0, 1)` as `SV_Position`.
- [x] Fragment shader: returns a `float4` color as `SV_Target`.
- [x] Keep in mind that clip-space +Y points down until milestone 3 flips Y with a negative viewport height.

## Created once

In the constructor of the frame pipeline.

- [x] `VkShaderModule` from the SPIR-V of the `CompiledShader`: one module for both entry points, destroyed once the pipeline is created.
- [x] `VkPipelineLayout`, empty: no descriptor set layouts, no push constant ranges.
- [ ] `VkPipeline` from `vkCreateGraphicsPipelines`:
  - [ ] Two `VkPipelineShaderStageCreateInfo` on the same module, with hard-coded entry point names until milestone 3.
  - [ ] Empty `VkPipelineVertexInputStateCreateInfo`.
  - [ ] Input assembly: `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`.
  - [ ] Viewport state: `viewportCount = 1`, `scissorCount = 1`, values left null.
  - [ ] Dynamic state: `VK_DYNAMIC_STATE_VIEWPORT` and `VK_DYNAMIC_STATE_SCISSOR`, so that `resize` doesn't rebuild the pipeline.
  - [ ] Rasterization: `VK_POLYGON_MODE_FILL`, `VK_CULL_MODE_NONE`, `lineWidth = 1.0f`.
  - [ ] Multisample: `VK_SAMPLE_COUNT_1_BIT`.
  - [ ] Color blend: one attachment, blending off, `colorWriteMask` set to RGBA (nothing is written if it is zero).
  - [ ] No depth/stencil state.
  - [ ] `VkPipelineRenderingCreateInfo` in `pNext`, with `colorAttachmentCount = 1` and the color image's format. `renderPass` is `VK_NULL_HANDLE`.

## Created on resize

- [ ] Color image through `IImageFactory::createImage`, e.g. `VK_FORMAT_R8G8B8A8_UNORM`, with usage `COLOR_ATTACHMENT | TRANSFER_SRC`. Its format must match the one given to the pipeline.
- [ ] Its `IImageView`, for the rendering attachment.

One color image is enough with several frames in flight: there is a single queue, and barriers order the frames.

## Recorded each frame

In `prepareExecution`.

- [ ] Barrier: color image to `COLOR_ATTACHMENT_OPTIMAL`, destination stage `COLOR_ATTACHMENT_OUTPUT`, access `COLOR_ATTACHMENT_WRITE`.
- [ ] `vkCmdBeginRendering` with a `VkRenderingInfo`:
  - [ ] `renderArea` set to the viewport size.
  - [ ] One `VkRenderingAttachmentInfo`: the image view, `loadOp = CLEAR`, `storeOp = STORE`, a clear color.
- [ ] `vkCmdBindPipeline`.
- [ ] `vkCmdSetViewport` and `vkCmdSetScissor`.
- [ ] `vkCmdDraw(3, 1, 0, 0)`.
- [ ] `vkCmdEndRendering`.
- [ ] Barriers: color image to `TRANSFER_SRC_OPTIMAL`, output image to `TRANSFER_DST_OPTIMAL`.
- [ ] Blit the viewport region to the output image.
- [ ] Leave the output image in the same final state as `AnariFramePipeline` does, as `ImguiRenderPanel` expects it.

## Not needed yet

Render passes, framebuffers, descriptors, vertex and index buffers, a depth image, a swapchain (owned by ImGui), and fences or semaphores of our own (the command queue handles them).
