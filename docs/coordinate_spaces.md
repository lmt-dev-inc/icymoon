# Coordinate Spaces

This document describes the coordinate spaces used in IcyMoon and the conventions each one follows.

- [Coordinate Spaces](#coordinate-spaces)
  - [Clip space and normalized device coordinates](#clip-space-and-normalized-device-coordinates)
    - [Use in GUI events](#use-in-gui-events)

## Clip space and normalized device coordinates

**Clip coordinates** are the 4D homogeneous coordinates `(x, y, z, w)` produced by the projection matrix (`PerspectiveProjection` in `im3e/utils/camera_transforms.h`) applied to view-space positions. Dividing by `w` (the perspective divide) gives **normalized device coordinates (NDC)**, where the visible volume is a fixed box whatever the camera, viewport size or resolution.

IcyMoon follows GLM's right-handed convention, with depth in [0, 1] (`GLM_FORCE_DEPTH_ZERO_TO_ONE` in the root `CMakeLists.txt`):

| Axis | Range in NDC | Direction                                               |
| ---- | ------------ | ------------------------------------------------------- |
| x    | [-1, 1]      | -1 is the left edge of the viewport, +1 the right edge  |
| y    | [-1, 1]      | **positive up**: -1 is the bottom edge, +1 the top edge |
| z    | [0, 1]       | 0 is the near plane, 1 the far plane                    |

Graphics APIs and libraries use different conventions, so be careful when you cross a boundary:

- **Vulkan**: NDC y points **down**. Vulkan pipelines that render with these matrices must flip y, for example with a negative viewport height.
- **ImGui and window pixels**: the origin is at the top-left corner and y points **down**, in pixels. Converting to NDC needs a scale, an offset and a y flip (see below).

### Use in GUI events

`IGuiEventListener` (`im3e/api/gui.h`) receives mouse movements in NDC units rather than in pixels. Listeners therefore do not depend on the GUI library, the panel size or the display scaling. They can also map movements straight into the scene with the inverse view-projection matrix, which is how `AnariMapCamera` pans.

