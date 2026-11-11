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
    - [5. One terrain tile](#5-one-terrain-tile)
    - [6. Lighting and basic PBR](#6-lighting-and-basic-pbr)
    - [7. Streamed terrain](#7-streamed-terrain)
    - [8. Remove ANARI](#8-remove-anari)
    - [9. Background tile loading](#9-background-tile-loading)
    - [10. Screen-space level of detail](#10-screen-space-level-of-detail)
  - [Testing](#testing)
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

| Library              | Folder                | Content                                                                   | Created in milestone |
| -------------------- | --------------------- | ------------------------------------------------------------------------- | -------------------- |
| `im3e_shader`        | `im3e/shader/`        | C++ types for compiled shaders and reflection data, shared Slang modules. | 1                    |
| `im3e_vulkan`        | `im3e/vulkan/`        | Gains shader modules, pipelines, depth images and buffers.                | exists               |
| `im3e_render_vulkan` | `im3e/render/vulkan/` | Frame pipeline, object renderers and their shaders.                       | 2                    |
| `im3e_scene`         | `im3e/scene/`         | Gains scene content: planes, terrains, light.                             | exists               |
| `im3e_terrain`       | `im3e/terrain/`       | Tile mesh generation, then tile selection and cache.                      | 5                    |

All follow the [library folder rules](../development/code_standards.md#library-folders). `render/` is a group folder, as described in [Families of libraries](../development/code_standards.md#families-of-libraries), so headers are included as `<im3e/render/vulkan/...>`.

`im3e_vulkan` links `im3e_shader` to create pipelines from its reflection data. `im3e_shader` itself depends on neither Vulkan nor Slang, as it only holds plain data, so simulation or another backend can use it too.

`im3e_terrain` is separate from `im3e_scene` because they answer different questions. The scene says what exists, as plain data: this height map, this plane. Streaming depends on a view: which tiles this camera needs, what is cached, what is being loaded. Two views of one scene need different tiles, so tile state can't be scene state. It is also separate from the renderer because it has no GPU code, and can be unit tested without mocking Vulkan.

### Shaders

- **Compiled at build time, through CMake.** The Slang library is used by a build tool only. It is not shipped with the project or its future Python module.
- **Compiled by a small tool of ours, `im3e_shader_compiler`,** in `im3e/shader/compiler/`, built on the Slang compilation API. It writes generated C++ with the SPIR-V and the reflection data, and a dependency file, which CMake passes to the build system through `add_custom_command(DEPFILE)`. This is preferred over `slangc` because reflection needs the API (see [Reflection](#reflection)), and it replaces a separate embedding step. The same code can later become a runtime compiler in `im3e_shader`, if hot reload or user-written shaders are wanted.
- **Listed like C++ files.** A `.slang` file with entry points goes in the `SOURCES` of its target, and the CMake function adds the build step. CMake has no global rule per file type, and one would not work here anyway, because only some `.slang` files have entry points.
- **Entry-point files are like `.cpp` files, imported modules are like headers.** Imported modules are not listed: the dependency file makes their users rebuild when they change.
- **Shaders live where they are used.** Entry-point files go in the `src/` folder of their library. Only modules shared between libraries go in `im3e_shader`, under `include/im3e/shader/`.
- **Imports follow the link graph.** A target can only import the shared modules of libraries it links.
- **Shaders are embedded in the binaries**, so that no shader file has to be found at run time (finding X3 of the review).

### Reflection

Reflection data is produced at build time and embedded with the SPIR-V, as plain C++ data. Code creating a pipeline only states what the shader can't know: the render state and the attachment formats.

| Written by hand today                  | Comes from reflection                                  | Milestone |
| -------------------------------------- | ------------------------------------------------------ | --------- |
| Shader stages and entry point names    | Entry points of the compiled shader                    | 3         |
| Pipeline layout, push constant ranges  | Parameters of the shader                               | 3         |
| C++ struct matching the push constants | Generated struct, with `static_assert`s on its offsets | 3         |
| Vertex input attributes                | Vertex inputs of the entry point                       | 5         |
| Descriptor set layouts                 | `ParameterBlock<T>` parameters of the shader           | 6         |

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
- **Deleted at parity** (milestone 8).
- Logic the new renderer needs is rewritten in the new libraries, leaving ANARI untouched. The temporary duplication disappears with the deletion.
- ANARI still has to compile when shared interfaces change, such as `IFramePipeline`. Changes to those are best kept for after milestone 8 where possible.

## Changes to the Architecture Review

| Review                                                                       | This plan                                                                                                                                                         |
| ---------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Step 5: renderer-neutral interfaces replace `IAnari*`, ANARI is one backend. | No neutral interfaces and no `im3e_render_api` until a second backend exists. The review's own argument against early abstractions applies.                       |
| Step 5: the new renderer is validated by image comparison with ANARI.        | ANARI uses ray tracers (`visrtx`, `helide`), so a rasterizer can't match it pixel for pixel. Comparison is visual, and tests check the new renderer's own output. |
| Step 4: logic moves out of `AnariHeightField` and `AnariWorld`.              | The logic is rewritten in new libraries, and ANARI is left as it is until it is deleted.                                                                          |
| Target architecture: `im3e_render_anari`.                                    | Never created.                                                                                                                                                    |
| Target architecture: quad tree in `im3e_terrain`.                            | Kept, but only moved in milestone 8, as ANARI uses it until then.                                                                                                 |
| Open question: WebGPU as the main native API.                                | Closed for now: the renderer is Vulkan.                                                                                                                           |

Once this plan is accepted, the review's roadmap should point to it.

## Milestones

| #   | Milestone                    | Depends on | What can be observed                                              |
| --- | ---------------------------- | ---------- | ----------------------------------------------------------------- |
| 1   | Shader compilation           |            | A `.slang` file builds like a `.cpp` file.                        |
| 2   | Triangle                     | 1          | A triangle in the viewer and in a headless test.                  |
| 3   | Camera and reflection        | 2          | Orbiting the triangle with the mouse.                             |
| 4   | Planes in a scene            | 3          | Two intersecting planes, with correct depth, editable in the GUI. |
| 5   | One terrain tile             | 4          | The coarsest tile of a real height map.                           |
| 6   | Lighting and basic PBR       | 5          | Lit plane and terrain, with editable materials.                   |
| 7   | Streamed terrain             | 5          | Navigating a full height map, with a level of detail property.    |
| 8   | Remove ANARI                 | 6, 7       | ANARI is gone from the code, the build and the dependencies.      |
| 9   | Background tile loading      | 7          | Tile loading no longer stalls frames.                             |
| 10  | Screen-space level of detail | 9          | Detail depends on the distance to the camera.                     |

Milestones 6 and 7 can be done in either order. Milestones 9 and 10 are not needed to remove ANARI.

### 1. Shader compilation

**Goal:** a `.slang` file listed in a target's sources is compiled to SPIR-V and embedded in that target, with the same incremental behaviour as a `.cpp` file.

Work:

1. Make the Slang library available through Conan: `recipes/slang` packages the official release binaries for Windows and Linux.
2. Create `im3e_shader`, with the C++ type that gives access to a compiled shader.
3. Create `im3e_shader_compiler` in `im3e/shader/compiler/`, which compiles one file with the Slang API, with all its entry points in one SPIR-V module.
   - It writes a `.h` and a `.cpp` that give access to the SPIR-V through the type of `im3e_shader`, and a dependency file listing the imported modules. Outputs are only rewritten when their content changes.
   - It prints the diagnostics of Slang as they are, in the usual `file(line): error` form, and returns non-zero on error.
   - `test_im3e_shader` tests it, with an `add_custom_command` written by hand for now. The command runs the tool with the `bin/` folder of Slang on the `PATH`, as Slang loads `slang-glslang` and its modules from its own folder.
   - The tool should be covered by the coverage report, if its instrumented runs during the build allow it.
   - Paths with non-ASCII characters aren't supported on Windows, as `argv` uses the process code page. The tool documents it.
4. Add `cmake/im3e_shaders.cmake`, and have `im3e_add_library` and the test target functions accept `.slang` files in their sources. It replaces the command written by hand. Debug builds keep shader debug information.
5. Add a shared module to `im3e_shader`, and pass the shared module folders of linked libraries to the tool as import paths.
6. Describe where shaders go in the [code standards](../development/code_standards.md), including the limitation on non-ASCII paths.

**Done when:**

- A unit test checks the SPIR-V magic number of an embedded shader that imports a shared module.
- Editing that shared module rebuilds only the shaders that import it.
- All Windows and Linux presets build.

### 2. Triangle

**Goal:** the whole path works, from a Slang shader to the screen and to a headless test. The triangle's vertices are hard-coded in its vertex shader.

Work:

1. Make the ray tracing extensions optional. A rasterizer doesn't need them, and requiring them rejects many GPUs and CPU implementations such as lavapipe.
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

### 5. One terrain tile

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

### 6. Lighting and basic PBR

**Goal:** planes and terrain are lit by a directional light, with basic PBR materials. Based on the GLSL BRDF of a previous project.

Work:

1. Add a directional light to the scene, and a material (base color, metallic factor, roughness factor, emissive factor) to planes and terrains, published as properties.
2. Port the BRDF from GLSL to a Slang module, imported by the plane and terrain shaders.
3. Move data shared by the frame (camera, light) to a parameter block backed by one buffer per frame in flight. Camera, transform, light and material together exceed the 128 bytes of push constants that Vulkan guarantees, while transform and material alone fit, so they stay per-draw push constants. `prepareExecution` has no frame index, so the pipeline counts frames itself, using the count from `resize`.
4. Add descriptor set layouts from reflection, and the descriptor allocation this needs, to `im3e_vulkan`.

**Done when:**

- Changing the light and the materials in the properties panel updates the viewer.
- An integration test checks the shading of a plane for known light directions and materials, including an emissive one.

### 7. Streamed terrain

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

### 8. Remove ANARI

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

### 9. Background tile loading

**Goal:** reading tiles no longer stalls frames. Tiles are currently read during `prepareExecution`, despite the name `updateAsync`.

Work:

1. Load tiles and generate their meshes on a worker thread. `IHeightMap` makes no thread-safety promise, so the worker should be the only user of the height map.
2. Hand finished tiles to the terrain renderer, and keep drawing the existing tiles until they arrive.
3. Limit the amount of data uploaded per frame.

**Done when:**

- Unit tests cover cancelling and replacing pending requests.
- The stats panel shows no tile reads on the render thread.

### 10. Screen-space level of detail

**Goal:** the level of detail of each tile depends on its screen-space error, replacing the single level chosen by a property.

Work:

1. Select tiles by screen-space error in `im3e_terrain`.
2. Hide the cracks between neighbouring tiles of different levels, with skirts or stitched indices.

**Done when:**

- Unit tests cover selection for known camera positions.
- No cracks are visible while navigating.

## Testing

- **Integration tests are headless, from milestone 2.** `createDevice` works without a window, and `DeviceIntegrationTest` already uses it that way. `PipelineIntegrationTest` runs an `IFramePipeline` into an image and checks pixels and pixel regions with a tolerance. Validation errors are logged as errors, which fail the tests.
- **Integration tests don't run in CI yet:** the workflows only run the `unit_test` label. Once ray tracing is optional (milestone 2), they could run on lavapipe. This is worth a separate small issue right after that milestone, so that each following milestone is covered.
- **Unit tests** use `MockVulkanFunctions` and the mocks of each library, and run without a GPU.
- `im3e_test_utils` is used as it is. Splitting it (finding T1 of the review) is a separate topic.
- Comparing whole images against stored references can be added if pixel checks turn out not to be enough.

## Later

Not part of this plan, but known to be coming:

- **Precision on large terrains.** Depth precision and floating-point precision will both become problems, and need a design for coordinate spaces, including how GPS or equivalent coordinates fit in when overlaying data from different sources.
- **Compute shaders and simulation.** Compute pipelines are added to `im3e_vulkan` with their first user. `im3e_shader` and the reflection path already work for them.
- **Prettier rendering effects**, such as shadows or image-based lighting, as new features of the renderer.
- **WebGPU**, once the project is mature and published.
