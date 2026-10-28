# Architecture Review

This document reviews the architecture described in [Architecture](../architecture/index.md): where it is inconsistent, how it compares with similar projects, and how it can evolve to stay maintainable as the project grows and replaces ANARI with its own renderers. It ends with a roadmap of the proposed changes, ordered so that each step is useful on its own.

- [Architecture Review](#architecture-review)
  - [Summary](#summary)
  - [Strengths to keep](#strengths-to-keep)
  - [Findings](#findings)
    - [Layering](#layering)
    - [Module boundaries and naming](#module-boundaries-and-naming)
    - [Ownership](#ownership)
    - [Tests](#tests)
    - [Leftovers](#leftovers)
  - [Library granularity](#library-granularity)
  - [Rendering backends](#rendering-backends)
    - [Replacing ANARI with a Vulkan renderer](#replacing-anari-with-a-vulkan-renderer)
    - [Adding a WebGPU renderer](#adding-a-webgpu-renderer)
    - [Where to put the seam](#where-to-put-the-seam)
  - [Target architecture](#target-architecture)
  - [Roadmap](#roadmap)
  - [Open questions](#open-questions)

## Summary

The building blocks are sound: interfaces with factory functions, RAII around every C handle and GPU operation, a properties system that keeps GUI code out of rendering, and a good split between unit and integration tests. The issues are in how the blocks are cut and layered:

- Vulkan leaks into layers that have nothing to do with the GPU, through `im3e_utils` and `im3e_api`.
- Interfaces live in three different kinds of places, and module boundaries are not enforced by the build.
- Logic that does not depend on the renderer (camera navigation, terrain tile selection and streaming) lives inside the ANARI module, and the scene is owned by the frame pipeline.

The last point is what matters most for the plan to replace ANARI: today, a new renderer would have to reimplement the camera and the terrain streaming, and applications are written against ANARI-named interfaces. The proposed fix is a renderer-independent scene layer, with renderers as backends that sync from it, rather than a generic graphics API abstraction.

## Strengths to keep

- **Interfaces and factory functions.** Clients, tests and future language bindings depend only on abstract interfaces.
- **RAII everywhere.** C handles are owned by `UniquePtrWithDeleter`, `VkUniquePtr` or `VkSharedPtr`; commands, barriers and stats spans are scoped objects.
- **Automatic image barriers.** Images track their layout and last access, so callers only state the destination.
- **Properties.** Objects publish `IPropertyGroup`s and the GUI generates controls for them; rendering code never includes ImGui.
- **`IFramePipeline` as a composition point.** The GUI renders any pipeline into a panel, including nested ones.
- **Input in NDC.** `IGuiEventListener` is independent of the GUI library, panel size and display scaling.
- **Dynamic Vulkan loading** (`VK_NO_PROTOTYPES`), which makes every Vulkan call mockable.
- **Unit and integration tests** labelled separately, with fixtures that fail on logged errors.

## Findings

Each finding has an identifier that the [roadmap](#roadmap) refers to.

### Layering

**L1. Vulkan reaches the foundation and data layers.** `vk_utils.h` lives in `im3e_utils` and includes `vk_mem_alloc.h`, so `im3e_utils` publicly links the Vulkan headers and VMA. `im3e_api` publicly links `im3e_utils`. As a result, `im3e_geo`, and anything that uses `IHeightMap`, compiles against Vulkan and VMA without using them.

**L2. `im3e_api` mixes GPU-specific and GPU-neutral contracts.** `device.h`, `image.h`, `command_buffer.h` and `vulkan_functions.h` are Vulkan interfaces (over 100 uses of `Vk*` types). `height_map.h`, `window.h` and most of `gui.h` are not. `IFramePipeline` and `IGuiPanel` take an `ICommandBuffer`, and `IGuiPanel::onWindowResized` takes a `VkFormat`, so the GUI contracts are tied to Vulkan too.

**L3. `im3e_utils` mixes unrelated layers.** It holds foundation services (loggers, stats), rendering math (camera transforms, view frustum) and Vulkan helpers. `core` and `properties` are subfolders of `utils`, but `utils` itself depends on `core`, so the folder tree does not match the dependency tree.

**L4. Renderer-independent logic lives in the ANARI module.**
- `AnariMapCamera` mixes map navigation (orbit state, mouse handling), camera state (projection and view transform, frustum) and ANARI parameter updates.
- `AnariHeightField` holds tile selection, the tile pool and the reuse policy, which any renderer of height maps needs.
- `AnariWorld::updateAsync` and `AnariHeightField::updateAsync` take an `AnariMapCamera`, when all they need is a view frustum.

**L5. Application code depends on ANARI-named interfaces.** `IAnariFramePipeline`, `IAnariWorld` and `IAnariObject` have nothing specific to ANARI apart from their names, but every application that uses them is tied to the ANARI module.

### Module boundaries and naming

**B1. Interfaces live in three kinds of places.** In the central `im3e_api` (device, images, GUI, height maps), in the module's own header (`IAnariEngine`, `IAnariWorld` in `anari.h`), in a dedicated `api` sub-target (`im3e_utils_properties_api`), and next to their implementation (`ILogger`, `IStatsProvider` in `im3e_utils`). There is no rule for choosing.

**B2. Boundaries are not enforced by the build.** Every target adds the repository root as a public include directory, so any target can include any header, including another module's `src/`, without linking it. No module does it today, but only by discipline. Inside a module, the public header is included both as `"anari.h"` (in `src/`) and as `<im3e/anari/anari.h>`.

**B3. Library types are inconsistent.** Most targets are `STATIC`, `im3e_vulkan_loaders` is `OBJECT`, and `im3e_guis` has no type, so `BUILD_SHARED_LIBS` would make only that one shared.

**B4. Names do not describe content.** `devices` is Vulkan only; `guis` is GLFW, Vulkan and ImGui; `im3e/resources` and the top-level `resources/` are different things; `im3e/usd` is not a library. Folder names mix plural and singular (`devices`, `guis`, `geo`, `anari`).

**B5. Mocks live in three kinds of places:** the API mocks in a top-level `im3e/mock/` rather than next to `api/`, and the others in `utils/mock/` and `devices/vulkan_loaders/mock/`.

**B6. All dependencies are required.** `find_package(anari REQUIRED)` and `find_package(GDAL REQUIRED)` make ANARI and GDAL mandatory even for code that does not use them, such as a future headless Python module.

### Ownership

**O1. The window application creates the device.** `GlfwWindowApplication` calls `createDevice`, and applications get the device from it. There is no way to create a device without GLFW, which blocks headless and offscreen rendering.

**O2. The scene and camera are owned by the frame pipeline.** `AnariFramePipeline` creates its own `AnariWorld` and `AnariMapCamera`. One scene cannot be shown in two views, and the camera cannot outlive the pipeline.

**O3. The camera state has two sources of truth.** `AnariMapCamera` derives the view transform from its orbit parameters (target, distance, tilt, heading), but the view transform's position and direction are also editable in the GUI. Edits in the GUI are overwritten by the next mouse interaction, and the orbit parameters are not editable.

### Tests

**T1. `im3e_test_utils` publicly links `im3e_devices` and `im3e_guis`.** The unit tests of `im3e_utils_core`, at the bottom of the dependency graph, link Vulkan, GLFW and ImGui.

**T2. Coverage gaps.** `im3e_anari` has no tests, and `im3e_geo` only tests the quad tree. Part of the ANARI module cannot be unit tested without mocking ANARI because of L4.

### Leftovers

**X1.** `gdal_dem_loader` and `anari_dem_surface_generator` in `applications/anari/` are built but no longer called, and duplicate `im3e_geo`.

**X2.** `im3e_usd_viewer` and `im3e/usd` are placeholders.

**X3.** `resources.h` hard-codes absolute source-tree paths for assets, which will not work once the library is installed.

## Library granularity

Splitting the library into several CMake targets is the right choice, for three reasons:

- **Optional heavy dependencies.** GDAL, ANARI, GLFW, ImGui and OpenUSD should each stay behind a target, so that a consumer only pays for what it uses. A Python user who only renders offscreen should not need GLFW.
- **Enforced layering.** A target can only use what it links, once include directories are scoped (B2).
- **Build and test isolation.**

The cuts should follow layers and dependencies rather than folders. The [target architecture](#target-architecture) has about ten product libraries, each with a clear layer and set of third-party dependencies:

- Very fine targets that have a single user become internal parts of that user: `im3e_vulkan_loaders` only serves the Vulkan device library, and can stay a separate seam for mocking without being a public target.
- `im3e_utils_properties_api` only needs to stay separate if a consumer needs the interfaces without the implementations.

For distribution, the static libraries should be linked into **one shared artifact per binding**, e.g. a single Python extension module. Several extension modules that each link the same static libraries would each get their own copy of globals (the GDAL instance, logger trackers, ImGui contexts), which causes subtle bugs. The alternatives are one extension module or one shared core library.

## Rendering backends

### Replacing ANARI with a Vulkan renderer

The architecture is **partly ready**.

What already works:
- `IFramePipeline` is the right seam: `ImguiRenderPanel` accepts any pipeline, so a Vulkan terrain pipeline can sit next to the ANARI one.
- The GPU infrastructure (device, images, command buffers, barriers, presenter) is already Vulkan. A native renderer also removes the CPU copy of each ANARI frame.
- Properties and NDC input events carry over unchanged.

What blocks it:
- Applications use ANARI-named interfaces (L5).
- Camera navigation and terrain streaming would have to be reimplemented in the new renderer (L4).
- There is no way to feed the same scene to both renderers, which would be the best way to validate the new one against ANARI (O2).

### Adding a WebGPU renderer

The architecture is **not ready**. Vulkan is part of the contracts: `IFramePipeline::prepareExecution` takes an `ICommandBuffer`, a `VkExtent2D` and an `IImage`; `IGuiPanel::draw` takes an `ICommandBuffer`; `IDevice` exposes Vulkan handles; the presenter and the ImGui back-end are Vulkan.

### Where to put the seam

A graphics API abstraction (an RHI, render hardware interface) that hides both Vulkan and WebGPU is a large project, and designing one before a second backend exists usually produces the wrong abstraction. The seam should instead sit higher, at the scene:

1. **A renderer-independent scene.** `im3e_scene` holds the scene content (terrains, planes, lights) and cameras as plain data with change tracking. Renderers sync from it and do not own it. Applications become:

   ```cpp
   auto pScene = createScene();
   pScene->addTerrain(loadHeightMapFromFile(...));
   auto pRenderer = createAnariRenderer(pDevice);  // or createVulkanRenderer(pDevice)
   auto pPipeline = pRenderer->createFramePipeline(pScene, pCamera);
   ```

   Switching backends is a one-line change, and rendering the same scene with ANARI and the new renderer gives an image-comparison test for the latter.

2. **Renderer-independent terrain streaming.** `im3e_terrain` selects tiles (eventually by screen-space error), manages the tile cache and loads tiles in the background. It outputs the tiles to add and remove with their mesh data; backends only upload and draw them.

3. **Camera controllers separate from cameras.** A `MapCameraController` implements `IGuiEventListener` and updates a `Camera`. No renderer handles input.

4. **A clearly named GPU layer.** `devices` and the Vulkan half of `im3e_api` become `im3e_vulkan`. `IHeightMap`, GUI events and window interfaces stay backend-neutral. A WebGPU backend becomes a sibling stack (`im3e_webgpu`, `im3e_render_webgpu`, ImGui's WebGPU back-end) that shares everything below the GPU layer and most of the ImGui panel code.

## Target architecture

Each layer depends only on the layers above it in this list, and nothing above the GPU layer includes a graphics API:

| Layer    | Library              | Content                                                                     | Replaces                                                                          |
| -------- | -------------------- | --------------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| core     | `im3e_core`          | Flags, paths, errors, platform, loggers, stats, math, view frustum.         | `im3e_utils_core`, most of `im3e_utils`                                           |
|          | `im3e_properties`    | Property interfaces and implementations.                                    | `im3e_utils_properties[_api]`                                                     |
| data     | `im3e_geo`           | `IHeightMap`, GDAL loading, overview pyramids. GDAL optional.               | `im3e_geo`, `height_map.h`                                                        |
|          | `im3e_terrain`       | Quad tree, level-of-detail selection, tile cache, background loading.       | quad tree from `im3e_geo`, logic from `AnariHeightField`                          |
| scene    | `im3e_scene`         | Scene content, cameras, camera controllers.                                 | logic from `AnariWorld`, `AnariMapCamera`                                         |
| GPU      | `im3e_vulkan`        | Loader, device, queues, images, commands, Vulkan helpers.                   | `im3e_devices`, `im3e_vulkan_loaders`, Vulkan part of `im3e_api` and `im3e_utils` |
| render   | `im3e_render_anari`  | ANARI backend: syncs a scene into ANARI objects.                            | `im3e_anari`                                                                      |
|          | `im3e_render_vulkan` | Native Vulkan backend.                                                      | new                                                                               |
| UI       | `im3e_gui`           | ImGui workspace and panels, independent of the GPU back-end where possible. | most of `im3e_guis`                                                               |
|          | `im3e_gui_glfw`      | GLFW windows, presenter, ImGui platform and renderer back-ends.             | rest of `im3e_guis`                                                               |
| bindings | `python/`            | One extension module.                                                       | new                                                                               |

Alongside the layering:

- Each library has its own `include/im3e/<name>/` and `src/` folders, and only its `include/` folder is public.
- Each library has a namespaced alias (`im3e::geo`), and installs and exports with a CMake package configuration.
- Optional libraries are enabled with `IM3E_WITH_*` options.
- Mocks live next to the interfaces they mock, in a `mock/` folder of the same library.

## Roadmap

Steps are ordered so that the cheapest and most valuable come first, and each one leaves the project in a working state.

| Step | Change                                                                                                                                                                                                                                                                                                                                     | Addresses          |
| ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------ |
| 0    | **Split the camera** (ticket #72): renderer-independent camera and map camera controller with unit tests, in a new `im3e_scene` library that also takes `PerspectiveProjection` and `ViewTransform` from `im3e_utils` (which uses properties without linking them); ANARI camera reduced to an adapter; height fields take a view frustum. | L4, O3, T2         |
| 1    | **CMake hygiene:** explicit library types, namespaced aliases, `IM3E_WITH_*` options with ANARI and OpenUSD optional, mocks next to their interfaces, split `im3e_test_utils` into GoogleTest helpers and device/GUI fixtures, delete dead code in `applications/anari/`.                                                                  | B3, B5, B6, T1, X1 |
| 2    | **Get Vulkan out of the lower layers:** move `vk_utils` and the Vulkan interfaces of `im3e_api` into the Vulkan library, so that core, properties and geo are GPU-free.                                                                                                                                                                    | L1, L2, L3         |
| 3    | **Let the host create the device** and pass it to the window application. Enables headless use.                                                                                                                                                                                                                                            | O1                 |
| 4    | **Add `im3e_scene` and `im3e_terrain`:** scene independent of views, terrain selection and streaming moved out of `AnariHeightField`, background loading.                                                                                                                                                                                  | L4, O2             |
| 5    | **Renderer-neutral interfaces** replace `IAnari*`, with ANARI as one backend. Start the native Vulkan renderer against the same scene, validated by image comparison with ANARI.                                                                                                                                                           | L5                 |
| 6    | **Per-library `include/` layout, install and export**, when starting the Python bindings.                                                                                                                                                                                                                                                  | B1, B2, B4, X3     |

## Open questions

- **WebGPU as the main native API?** Dawn and wgpu-native run on Vulkan, Metal and D3D12, and the same code can target browsers. If reach matters more than Vulkan-only features (ray tracing, bindless resources, fine-grained synchronization), a single WebGPU renderer could replace both the planned Vulkan renderer and any abstraction over the two. This should be decided before investing in the native Vulkan renderer.
- **Who sets a camera's aspect ratio** once a camera can be shared by several views of different sizes: the camera, or the view that renders it?
- **How do renderers track scene changes:** change notifications (as properties do today) or version counters compared at frame boundaries? Version counters are simpler for renderers that sync once per frame and for background threads.
- **Bindings packaging:** one extension module linking the static libraries, or a shared core library with thin bindings?
