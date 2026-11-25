# Vulkan Renderer Plan

This document plans the native Vulkan renderer that will replace ANARI, with shaders written in Slang. It builds on the [Architecture Review](architecture_review.md) and changes it on a few points (see [Changes to the Architecture Review](#changes-to-the-architecture-review)).

The renderer grows through visible steps: a triangle, then a camera, a plane, a terrain tile, lighting, and a streamed terrain. Infrastructure is added by the first milestone that needs it, and shaped by that use. Each milestone is meant to become one GitHub issue. Its work items are roughly one commit each, and every milestone leaves the project building with its tests passing. Names of new types and folders are proposals.

- [Vulkan Renderer Plan](#vulkan-renderer-plan)
  - [Goal](#goal)
  - [Design decisions](#design-decisions)
    - [Libraries](#libraries)
    - [Shaders](#shaders)
    - [Reflection](#reflection)
    - [Renderer](#renderer)
    - [ANARI](#anari)
  - [Changes to the Architecture Review](#changes-to-the-architecture-review)
  - [Milestones](#milestones)
    - [1. Shader compilation](#1-shader-compilation)
    - [2. Triangle](#2-triangle)
    - [3. Camera and reflection](#3-camera-and-reflection)
    - [4. Planes in a scene](#4-planes-in-a-scene)
    - [5. Interactive integration tests](#5-interactive-integration-tests)
    - [6. One terrain tile](#6-one-terrain-tile)
    - [7. Lighting and basic PBR](#7-lighting-and-basic-pbr)
    - [8. Streamed terrain](#8-streamed-terrain)
    - [9. Remove ANARI](#9-remove-anari)
    - [10. Background tile loading](#10-background-tile-loading)
    - [11. Screen-space level of detail](#11-screen-space-level-of-detail)
  - [Testing](#testing)
    - [Splitting `im3e_test_utils`](#splitting-im3e_test_utils)
  - [Later](#later)

## Goal

Replace `AnariFramePipeline` with a lightweight rasterizer that can take prettier effects later, then delete ANARI. The new `viewer` application has to show:

- a height map streamed as tiles, at a level of detail chosen with a property;
- planes, to show that the renderer supports different types of objects;
- one directional light and basic PBR materials;
- the map camera and its mouse navigation;
- the properties and stats panels.

Out of scope: ray tracing, real-time simulation, Python bindings, and large-terrain precision (see [Later](#later)). WebGPU will only be considered once the project is mature and published.

## Design decisions

### Libraries

| Library                    | Folder                      | Content                                                                   | Created in milestone |
| -------------------------- | --------------------------- | ------------------------------------------------------------------------- | -------------------- |
| `im3e_shader`              | `im3e/shader/`              | C++ types for compiled shaders and reflection data, shared Slang modules. | 1                    |
| `im3e_vulkan`              | `im3e/vulkan/`              | Gains shader modules, pipelines, depth images and buffers.                | exists               |
| `im3e_render_vulkan`       | `im3e/render/vulkan/`       | Frame pipeline, object renderers and their shaders.                       | 2                    |
| `im3e_scene`               | `im3e/scene/`               | Gains scene content: planes, terrains, light.                             | exists               |
| `im3e_testing_interactive` | `im3e/testing/interactive/` | Runs the integration tests that render in a window.                       | 5                    |
| `im3e_terrain`             | `im3e/terrain/`             | Tile mesh generation, then tile selection and cache.                      | 6                    |

All follow the [library folder rules](../development/code_standards.md#library-folders). `render/` is a group folder, as described in [Families of libraries](../development/code_standards.md#families-of-libraries), so headers are included as `<im3e/render/vulkan/...>`. `testing/` is a group folder too, created by [splitting `im3e_test_utils`](#splitting-im3e_test_utils) before milestone 5.

`im3e_vulkan` links `im3e_shader` to create pipelines from its reflection data. `im3e_shader` itself depends on neither Vulkan nor Slang, as it only holds plain data, so simulation or another backend can use it too.

`im3e_terrain` is separate from `im3e_scene` because they answer different questions. The scene says what exists, as plain data: this height map, this plane. Streaming depends on a view: which tiles this camera needs, what is cached, what is being loaded. Two views of one scene need different tiles, so tile state can't be scene state. It is also separate from the renderer because it has no GPU code, and can be unit tested without mocking Vulkan.

### Shaders

- **Compiled at build time, through CMake.** The Slang library is used by a build tool only. It is not shipped with the project or its future Python module.
- **Compiled by a small tool of ours, `im3e_shader_compiler`,** in `im3e/shader/compiler/`, built on the Slang compilation API. It writes generated C++ with the SPIR-V and the reflection data, and a dependency file, which CMake passes to the build system through `add_custom_command(DEPFILE)`. This is preferred over `slangc` because reflection needs the API (see [Reflection](#reflection)), and it replaces a separate embedding step. The same code can later become a runtime compiler in `im3e_shader`, if hot reload or user-written shaders are wanted.
- **Entry-point files are like `.cpp` files, imported modules are like headers.** A `.slang` file with entry points goes in the `SHADERS` of its target, and the CMake function adds the build step. Imported modules go in `SOURCES`, like headers: they are shown in IDEs but not compiled on their own, and the dependency file makes their users rebuild when they change.
- **Why a separate `SHADERS` list.** Both kinds of files must have the `.slang` extension, since `import` only finds modules by that name, so the extension can't tell them apart as it does for C++. Detecting entry points in CMake would read every shader at configure time and rerun CMake on each edit. Letting the tool decide doesn't work either, as the outputs of a build step must be known at configure time. The tool fails on a file without entry points, so a module listed in `SHADERS` by mistake is reported.
- **Shaders live where they are used.** Entry-point files go in the `src/` folder of their library. Modules shared between libraries go in the `include/` folder of the library that owns them, and general-purpose ones in `im3e_shader`, under `include/im3e/shader/`.
- **Imports follow the link graph.** The import paths of a shader are the include directories of its target, so a target can only import the shared modules of libraries it links, as for headers.
- **Shaders are embedded in the binaries**, so that no shader file has to be found at run time (finding X3 of the review).

### Reflection

Reflection data is produced at build time and embedded with the SPIR-V, as plain C++ data. Code creating a pipeline only states what the shader can't know: the render state and the attachment formats.

| Written by hand today                  | Comes from reflection                                  | Milestone |
| -------------------------------------- | ------------------------------------------------------ | --------- |
| Shader stages and entry point names    | Entry points of the compiled shader                    | 3         |
| Pipeline layout, push constant ranges  | Parameters of the shader                               | 3         |
| C++ struct matching the push constants | Generated struct, with `static_assert`s on its offsets | 3         |
| Vertex input attributes                | Vertex inputs of the entry point                       | 6         |
| Descriptor set layouts                 | `ParameterBlock<T>` parameters of the shader           | 7         |

The tool uses the Slang reflection API rather than `slangc -reflection-json`. The JSON would need a parser for a format we found no specification for, and the Slang user guide directs reflection users to the API.

### Renderer

- **Dynamic rendering** rather than render pass objects. It is core in Vulkan 1.3, which the device already requires.
- **The pipeline owns its color and depth images and blits to the output image**, as `AnariFramePipeline` does. The output image of `ImguiRenderPanel` only has the `TRANSFER_DST` and `SAMPLED` usages, so it cannot be a render target.
- **Geometry comes from the shader until real data needs a buffer.** The triangle and planes are generated in the vertex shader from `SV_VertexID`. Terrain tiles are the first use of buffers.
- **Push constants carry per-draw data.** Vulkan only guarantees 128 bytes, so data shared by a frame moves to a parameter block when lighting needs it.
- **Each object type has a small private renderer** (plane, terrain) inside `im3e_render_vulkan`, with its own shader and pipeline. New object types add one.
- **Uploads are recorded in the frame's command buffer.** The device has a [single queue](../architecture/vulkan.md#known-limitations), so a separate transfer path would not gain anything yet.
- **Y is flipped with a negative viewport height**, as described in [Coordinate Spaces](../architecture/coordinate_spaces.md).

### ANARI

- **Frozen.** `im3e/anari/` stays where it is and is not moved to `im3e/render/anari/`, or ported to the new scene.
- **Deleted at parity** (milestone 9).
- Logic the new renderer needs is rewritten in the new libraries, leaving ANARI untouched. The temporary duplication disappears with the deletion.
- ANARI still has to compile when shared interfaces change, such as `IFramePipeline`. Changes to those are best kept for after milestone 9 where possible.

## Changes to the Architecture Review

| Review                                                                       | This plan                                                                                                                                                         |
| ---------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Step 5: renderer-neutral interfaces replace `IAnari*`, ANARI is one backend. | No neutral interfaces and no `im3e_render_api` until a second backend exists. The review's own argument against early abstractions applies.                       |
| Step 5: the new renderer is validated by image comparison with ANARI.        | ANARI uses ray tracers (`visrtx`, `helide`), so a rasterizer can't match it pixel for pixel. Comparison is visual, and tests check the new renderer's own output. |
| Step 4: logic moves out of `AnariHeightField` and `AnariWorld`.              | The logic is rewritten in new libraries, and ANARI is left as it is until it is deleted.                                                                          |
| Target architecture: `im3e_render_anari`.                                    | Never created.                                                                                                                                                    |
| Target architecture: quad tree in `im3e_terrain`.                            | Kept, but only moved in milestone 9, as ANARI uses it until then.                                                                                                 |
| Step 1: split `im3e_test_utils` into helpers and fixtures (finding T1).      | Split into three libraries under `im3e/testing/` before milestone 5 (see [Splitting `im3e_test_utils`](#splitting-im3e_test_utils)).                              |
| Open question: WebGPU as the main native API.                                | Closed for now: the renderer is Vulkan.                                                                                                                           |

Once this plan is accepted, the review's roadmap should point to it.

## Milestones

| #   | Milestone                     | Depends on | What can be observed                                                 |
| --- | ----------------------------- | ---------- | -------------------------------------------------------------------- |
| 1   | Shader compilation            |            | A `.slang` file builds like a `.cpp` file.                           |
| 2   | Triangle                      | 1          | A triangle in the viewer and in a headless test.                     |
| 3   | Camera and reflection         | 2          | Orbiting the triangle with the mouse.                                |
| 4   | Planes in a scene             | 3          | Two intersecting planes, with correct depth, editable in the GUI.    |
| 5   | Interactive integration tests | 4          | An integration test of the planes, navigated and edited in a window. |
| 6   | One terrain tile              | 4          | The coarsest tile of a real height map.                              |
| 7   | Lighting and basic PBR        | 6          | Lit plane and terrain, with editable materials.                      |
| 8   | Streamed terrain              | 6          | Navigating a full height map, with a level of detail property.       |
| 9   | Remove ANARI                  | 7, 8       | ANARI is gone from the code, the build and the dependencies.         |
| 10  | Background tile loading       | 8          | Tile loading no longer stalls frames.                                |
| 11  | Screen-space level of detail  | 10         | Detail depends on the distance to the camera.                        |

Milestone 5 also depends on [splitting `im3e_test_utils`](#splitting-im3e_test_utils), a separate issue. It comes before the terrain milestones so that their integration tests can be evaluated in a window, but none of them strictly needs it. Milestones 7 and 8 can be done in either order. Milestones 10 and 11 are not needed to remove ANARI.

### 1. Shader compilation

**Goal:** a `.slang` file listed in a target's shaders is compiled to SPIR-V and embedded in that target, with the same incremental behaviour as a `.cpp` file.

Work:

1. Make the Slang library available through Conan: `recipes/slang` packages the official release binaries for Windows and Linux.
2. Create `im3e_shader`, with the C++ type that gives access to a compiled shader.
3. Create `im3e_shader_compiler` in `im3e/shader/compiler/app/` (see [Executables](../development/code_standards.md#executables)), which compiles one file with the Slang API, with all its entry points in one SPIR-V module.
   - It writes a `.h` and a `.cpp` that give access to the SPIR-V through the type of `im3e_shader`, and a dependency file listing the imported modules. Outputs are only rewritten when their content changes.
   - It prints the diagnostics of Slang as they are, with the file, line and column of each error, and returns non-zero on error.
   - `test_im3e_shader_compiler` tests it, with an `add_custom_command` written by hand for now. The command runs the tool with the `bin/` folder of Slang on the `PATH`, as Slang loads `slang-glslang` and its modules from its own folder.
   - The tests also read the dependency file: it must list the shader and the module it imports, with the generated files as targets. Escaping is tested in work item 4, with folders whose names have a space.
   - CTest tests run the tool on invalid inputs (a missing file, a module without entry points, a Slang error, an invalid function name), and check that it prints the error and returns non-zero. Two more run it successfully, with a function in a namespace and in the global namespace.
   - The tool is in the coverage report, from its runs under CTest. Its runs during the build aren't reported, as the coverage data of the build is deleted before the tests run.
   - Its arguments are parsed with CLI11, which also reads them as UTF-8 on Windows, where `argv` uses the process code page.
4. Add `cmake/im3e_shaders.cmake`, and give `im3e_add_library` and the test target functions a `SHADERS` argument for the `.slang` files with entry points. It replaces the command written by hand. Debug builds keep shader debug information.
   - A CTest test builds a small CMake project with two shaders, one importing a module, changes the module, and checks that only the importing shader is compiled again. The project has no C++ language, so the function must also be able to add the build step without a C++ target. Its folders have a space in their names, so that the build tool reads escaped paths from the dependency files.
5. Add a shared module to `im3e_shader`, and pass the include directories of the target to the tool as import paths, which hold the shared module folders of linked libraries.
6. Describe where shaders go in the [code standards](../development/code_standards.md).

**Done when:**

- A unit test checks the SPIR-V magic number of an embedded shader that imports a shared module.
- Editing that shared module rebuilds only the shaders that import it.
- All Windows and Linux presets build.

### 2. Triangle

**Goal:** the whole path works, from a Slang shader to the screen and to a headless test. The triangle's vertices are hard-coded in its vertex shader.

Work:

1. Remove the ray tracing extensions. A rasterizer doesn't need them, and requiring them rejects many GPUs and CPU implementations such as lavapipe. Nothing uses them, so they are added back with their first user. Device selection checks instead that the device supports Vulkan 1.3.
2. Enable `dynamicRendering`, and add the Vulkan functions this milestone uses to the loader tables and `MockVulkanFunctions`.
3. Add shader modules and a minimal graphics pipeline to `im3e_vulkan`: no vertex input, no depth, empty pipeline layout.
4. Create `im3e/render/vulkan/`, with a frame pipeline implementing `IFramePipeline` that draws the triangle in its own color image and blits it to the output image. Handle `resize`.
5. Add a headless integration test with `PipelineIntegrationTest`, checking pixels inside and outside the triangle.
6. Create the `viewer` application in `applications/viewer/`, with the render and stats panels.

**Done when:**

- The viewer shows the triangle, and resizing the panel produces no validation errors.
- The integration test passes.
- `im3e_render_vulkan` doesn't link ANARI.

### 3. Camera and reflection

**Goal:** the triangle sits in the world and the map camera navigates around it. Pipelines are created from reflection data.

Work:

1. Add entry points and push constants to the reflection output of the tool, and generate the C++ push constant struct.
2. Create pipeline layouts and shader stages from reflection data in `im3e_vulkan`.
3. Pass an `ICamera` to the frame pipeline, push its matrix, flip Y, and set its aspect ratio from the viewport as `AnariFramePipeline` does.
4. Add `MapCameraController` to the viewer, as described in [Camera](../architecture/camera.md).

**Done when:**

- The triangle can be orbited, panned and zoomed in the viewer.
- Unit tests check the reflection output of a known shader.
- An integration test checks the image from two camera positions.
- `im3e_render_vulkan` has no hand-written pipeline layout or push constant struct.

### 4. Planes in a scene

**Goal:** the first real object type, in a scene owned by the application (finding O2 of the review).

Work:

1. Add a scene to `im3e_scene` that holds planes, each with a transform, a size and a color published as properties.
2. Have the frame pipeline take a scene and a camera.
3. Add the private plane renderer, with its corners generated in the vertex shader.
4. Support depth images and add a depth attachment. Barriers currently hard-code the color aspect in [vulkan_command_buffer.cpp](../../im3e/vulkan/lib/src/vulkan_command_buffer.cpp).

**Done when:**

- The viewer shows two intersecting planes with correct depth, and they can be edited in the properties panel.
- An integration test checks pixels on both sides of the intersection.
- Unit tests cover the scene, and the scene doesn't include any renderer or graphics API header.

### 5. Interactive integration tests

**Goal:** any integration test that renders can also be run in a window, to navigate its scene, edit its properties, and debug or profile it with tools such as Nsight Graphics or RenderDoc. Many rendering features, such as tile streaming or level of detail selection, are best judged by looking at them while moving around, which pixel checks can't do.

Tests stay in the integration test executable of their library, and are chosen with the filter of GoogleTest, so this scales with the number of libraries and tests:

```
integration_im3e_render_vulkan --gtest_filter=VulkanFramePipelineIntegration.drawsPlanes --interactive
```

This avoids a design where every test registers in one list, shown by one application: it puts all integration tests in one executable, and makes them tedious to browse. GoogleTest already names each test by executable, suite and name, and IDE extensions such as C++ TestMate for VS Code show them as a tree and launch one with extra arguments.

Requires [splitting `im3e_test_utils`](#splitting-im3e_test_utils) first, so that the new library is created in its final place, and the test functions already choose the GoogleTest `main`.

Work:

1. Have `PipelineIntegrationTest` run its frames through a runner interface, with a headless runner as the default, so that `im3e_testing_vulkan` doesn't depend on a GUI.
2. Let tests give the fixture their scene and camera, so that a runner can show them. Tests that don't still run in a window, without navigation or properties.
3. Create `im3e_testing_interactive` in `im3e/testing/interactive/`, with a windowed runner: a window with a render panel running the test's pipeline, the properties and stats panels, and `MapCameraController` on the test's camera. The pipeline is resized with the panel. The library is named after what a test executable gains by linking it rather than after the GUI it uses.
4. Add a GoogleTest `main` to `im3e_testing_interactive`, which installs the windowed runner when `--interactive` is given. The test is reported as skipped once the window is closed, as its checks don't apply to an image that was resized and navigated.
5. Give `im3e_add_integration_tests_executable` an `INTERACTIVE` option that links this `main` instead of `gtest_main`, and use it for `integration_im3e_render_vulkan`. Executables without the option, and runs without `--interactive`, don't create a window, so headless runs such as CI are unchanged.
6. Describe how to run a test interactively in the development documents, including a VS Code launch configuration that asks for the test filter.

**Done when:**

- The integration test of the planes of milestone 4 runs in a window with `--interactive`, can be navigated, and its planes can be edited in the properties panel.
- Without `--interactive`, the integration tests create no window and pass as before.
- A frame of an interactive test can be captured with Nsight Graphics.
- Unit tests cover the choice of runner from the arguments.

### 6. One terrain tile

**Goal:** real data on screen: the coarsest tile of a height map, which covers the whole map. Tile shading only needs to make the shape readable at this point, for example by showing normals as colors.

Work:

1. Create `im3e_terrain`, with tile mesh generation from an `IHeightMapTileSampler`: positions and indices, skipping invalid samples. Based on [anari_height_field_tile.cpp](../../im3e/anari/src/anari_height_field_tile.cpp), which doesn't change.
2. Add what this tile needs to `im3e_vulkan`: device-local vertex and index buffers, a staging upload recorded in a given command buffer, and buffer barriers.
3. Add vertex inputs to the reflection output, and use them in pipeline creation.
4. Add terrains to the scene, with change tracking so that the renderer knows when to upload. Version counters compared at frame boundaries are recommended over notifications, as the renderer syncs once per frame.
5. Add the private terrain renderer and its shader. Tiles only have positions, so normals are computed in the fragment shader from position derivatives.
6. Have the viewer load the height map given on the command line, rather than a hard-coded path as in the ANARI viewer.

**Done when:**

- The viewer shows the tile of a real height map, next to a plane.
- Unit tests with `MockHeightMap` cover mesh generation, including invalid samples.
- An integration test renders a small synthetic height map and checks its pixels.
- `im3e_terrain` doesn't link Vulkan.

### 7. Lighting and basic PBR

**Goal:** planes and terrain are lit by a directional light, with basic PBR materials. Based on the GLSL BRDF of a previous project.

Work:

1. Add a directional light to the scene, and a material (base color, metallic factor, roughness factor, emissive factor) to planes and terrains, published as properties.
2. Port the BRDF from GLSL to a Slang module, imported by the plane and terrain shaders.
3. Move data shared by the frame (camera, light) to a parameter block backed by one buffer per frame in flight. Camera, transform, light and material together exceed the 128 bytes of push constants that Vulkan guarantees, while transform and material alone fit, so they stay per-draw push constants. `prepareExecution` has no frame index, so the pipeline counts frames itself, using the count from `resize`.
4. Add descriptor set layouts from reflection, and the descriptor allocation this needs, to `im3e_vulkan`.

**Done when:**

- Changing the light and the materials in the properties panel updates the viewer.
- An integration test checks the shading of a plane for known light directions and materials, including an emissive one.

### 8. Streamed terrain

**Goal:** the viewer navigates a full height map, as the ANARI viewer does. Based on [anari_height_field.cpp](../../im3e/anari/src/anari_height_field.cpp), which doesn't change.

Work:

1. Add tile selection from a `ViewFrustum` and a level of detail to `im3e_terrain`, using the quad tree of `im3e_geo`, and publish the level of detail as a property.
2. Add the tile cache: a fixed number of slots, reuse of tiles that are still loaded, and tiles to add and remove as output.
3. Remember empty tiles. Today a tile with no valid data is read again every frame, as `load` returns false and nothing records it.
4. Fix the culling problem noted in `AnariHeightField::updateAsync`: when zoomed out and switching levels of detail, some visible tiles are missing.
5. Add a GPU tile pool to the terrain renderer. Tile buffers are only reused once no frame in flight can use them.
6. Add stats spans.

**Done when:**

- The new viewer and the ANARI viewer show the same terrain, in a visual comparison from the same camera positions.
- Unit tests with `MockHeightMap` cover selection, reuse, eviction and empty tiles.
- Navigating and changing the level of detail produce no validation errors.

### 9. Remove ANARI

**Goal:** the new renderer is the only one.

Work:

1. Save screenshots of the ANARI viewer as a reference, in the GitHub issue rather than in the repository.
2. Delete `im3e/anari/` and `applications/anari/`.
3. Remove `recipes/anari`, the requirement in `conanfile.py`, `find_package(anari REQUIRED)`, and the ANARI parts of `cmake/im3e_runtime_dlls.cmake`.
4. Move the quad tree from `im3e_geo` to `im3e_terrain`.
5. Update the [architecture documents](../architecture/index.md), including the ANARI adapter section of [Camera](../architecture/camera.md).

**Done when:**

- A search for `anari` in code and build files finds nothing.
- All presets build from a clean Conan install.

### 10. Background tile loading

**Goal:** reading tiles no longer stalls frames. Tiles are currently read during `prepareExecution`, despite the name `updateAsync`.

Work:

1. Load tiles and generate their meshes on a worker thread. `IHeightMap` makes no thread-safety promise, so the worker should be the only user of the height map.
2. Hand finished tiles to the terrain renderer, and keep drawing the existing tiles until they arrive.
3. Limit the amount of data uploaded per frame.

**Done when:**

- Unit tests cover cancelling and replacing pending requests.
- The stats panel shows no tile reads on the render thread.

### 11. Screen-space level of detail

**Goal:** the level of detail of each tile depends on its screen-space error, replacing the single level chosen by a property.

Work:

1. Select tiles by screen-space error in `im3e_terrain`.
2. Hide the cracks between neighbouring tiles of different levels, with skirts or stitched indices.

**Done when:**

- Unit tests cover selection for known camera positions.
- No cracks are visible while navigating.

## Testing

- **Integration tests are headless, from milestone 2.** `createDevice` works without a window, and `DeviceIntegrationTest` already uses it that way. `PipelineIntegrationTest` runs an `IFramePipeline` into an image and checks pixels and pixel regions with a tolerance. Validation errors are logged as errors, which fail the tests.
- **Integration tests don't run in CI yet:** the workflows only run the `unit_test` label. Once ray tracing is no longer required (milestone 2), they could run on lavapipe. This is worth a separate small issue right after that milestone, so that each following milestone is covered.
- **Unit tests** use `MockVulkanFunctions` and the mocks of each library, and run without a GPU.
- **Integration tests that render can run in a window, from milestone 5**, to evaluate features by navigating them, and to debug or profile them. The headless checks remain the automated tests.
- Comparing whole images against stored references can be added if pixel checks turn out not to be enough.

### Splitting `im3e_test_utils`

`im3e_test_utils` is used as it is until milestone 5. Before that milestone, a separate issue splits it into a group of libraries, which closes finding T1 of the review: today every test links Vulkan, GLFW and ImGui through it, including the unit tests of `im3e_utils_core`.

- **`im3e_testing_core`** in `im3e/testing/core/`: the GoogleTest names imported into `im3e` (`test_utils.h`), printers and matchers for GLM and standard types, `IntegrationTest`, and the test output path helper. Depends on GoogleTest, glm, fmt and `im3e_utils`.
- **`im3e_testing_vulkan`** in `im3e/testing/vulkan/`: printers for Vulkan types, `DeviceIntegrationTest` and `PipelineIntegrationTest`, and later the runner interface of milestone 5 and the benchmark runner. Depends on `im3e_testing_core` and `im3e_vulkan`.
- **`im3e_testing_interactive`** in `im3e/testing/interactive/`, created by milestone 5: the windowed runner and its GoogleTest `main`. Depends on `im3e_testing_vulkan` and `im3e_guis`.

Design points:

- **`vulkan` and `interactive` stay separate.** `im3e_guis` depends on `im3e_vulkan`, so Vulkan fixtures that linked it would make the integration tests of `im3e_vulkan` link a library built on top of it. Executables without the `INTERACTIVE` option of milestone 5 must also not link GLFW and ImGui.
- **`im3e_guis` is dropped.** `im3e_test_utils` links it publicly, but none of its files includes a header of it.
- **The GoogleTest `main` comes from the test functions.** `im3e_test_utils` links `GTest::gtest_main` publicly, so every test executable gets its `main`, and `im3e_testing_interactive` couldn't provide another one. `im3e_add_unit_tests_executable` and `im3e_add_integration_tests_executable` link `gtest_main` instead, and milestone 5 makes the second one link the interactive `main` when asked.
- **`IntegrationTest` needs the loggers of `im3e_utils`**, so the unit tests of `im3e_utils_core` link `im3e_utils` through `im3e_testing_core`. That is much less than today. It can move to a library of its own once `im3e_utils` follows the library folder rules.
- **The issue touches every test.** About 80 files include `<im3e/test_utils/...>` or link `im3e_test_utils`, and the [code standards](../development/code_standards.md#tests) and architecture documents name them.

## Later

Not part of this plan, but known to be coming:

- **Precision on large terrains.** Depth precision and floating-point precision will both become problems, and need a design for coordinate spaces, including how GPS or equivalent coordinates fit in when overlaying data from different sources.
- **Compute shaders and simulation.** Compute pipelines are added to `im3e_vulkan` with their first user. `im3e_shader` and the reflection path already work for them.
- **Prettier rendering effects**, such as shadows or image-based lighting, as new features of the renderer.
- **Benchmarks of integration tests**, as a third runner in `im3e_testing_vulkan`, next to the headless and windowed ones of milestone 5: rendering a number of frames headless and reporting GPU timings from timestamp queries.
- **WebGPU**, once the project is mature and published.
