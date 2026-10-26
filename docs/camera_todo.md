# Camera Todo (ticket #72)

Temporary progress tracker: delete before creating the PR.

## 1. Ray fix (in `utils`)

- [x] Move `Ray` out of `im3e/utils/camera_transforms.h` into a new `im3e/utils/ray.h`, with its implementation in `src/ray.cpp`.
- [x] Fix `Ray::createViewRayFromNdc`: the near point should be at NDC z = 0, not -1, because depth is [0, 1].
- [ ] `Ray::createViewRayFromNdc` should be moved to new `Camera` class and tested there
- [ ] Add `test_ray.cpp` to `test_im3e_utils`:
  - NDC (0, 0) gives a ray along the view direction, with its origin on the near plane.
  - Corner NDC positions give rays through the corners of the frustum.

## 2. `im3e_scene` library

- [x] Create `im3e/scene/` with an `im3e_scene` STATIC target. It links `im3e_utils` and `im3e_utils_properties` explicitly; add it to `im3e/CMakeLists.txt`.
- [ ] Move `PerspectiveProjection` and `ViewTransform` there, then delete `camera_transforms.h` and its `.cpp` from `utils`.
- [ ] Add `Camera`:
  - It owns a projection and a view transform.
  - Its `ViewFrustum` is rebuilt lazily from a dirty flag, so it is never stale.
  - It has a `getVersion()` counter that increments on every change.
  - `createProperties()` exposes position, direction, up and right as read-only.
- [ ] Add tests:
  - `test_perspective_projection.cpp`: the matrix matches `glm::perspective`, values are clamped to their min/max, and setters fire the change callback.
  - `test_view_transform.cpp`: `setDirection` makes the axes orthonormal, the matrix matches `glm::lookAt`, and setters fire the change callback.
  - `test_camera.cpp`: the frustum follows changes to the aspect ratio, FOV, near/far and position; the version increments on each change and stays the same when nothing changes.

## 3. `MapCameraController` (in `im3e_scene`)

- [ ] Add `MapCameraState` (target, distance, tilt, heading) and `MapCameraController : IGuiEventListener`, with `setState` and `getState`.
- [ ] Move the navigation logic out of `AnariMapCamera`: drag constants, `intersectHorizontalPlane`, the pose computation, and the pan, tilt, heading and zoom handling.
- [ ] Decide on zoom: exponential (`distance *= pow(0.9F, steps)`) or clamped to a min/max distance. Either way the distance must never reach zero or go negative.
- [ ] Make the initial state a constructor argument instead of the hard-coded target (3700, 0, 4000) and distance 5000.
- [ ] Add `createProperties()` exposing the state as editable values (target, distance, tilt, heading), with clamping through `setState`.
- [ ] Add `test_map_camera_controller.cpp`, with a `makeDrag(prev, curr, buttons, modifiers)` helper that keeps `ndcOffset` equal to `curr - prev`:
  - **Initial pose:** tilt 0 looks straight down at the target from `distance`.
  - **Pan:** the ground point under `prev` ends up under `curr`, and the target's height stays the same.
  - **Pan above the horizon:** a drag whose ray doesn't hit the plane leaves the state unchanged.
  - **Tilt:** a vertical Ctrl+drag changes tilt, clamped to [0, 70°].
  - **Heading:** a full-width Ctrl+drag makes a full turn and comes back to the same heading.
  - **No left button:** nothing changes.
  - **Zoom:** one wheel step applies the expected factor, and a large scroll keeps the distance positive.
  - **`setState`:** clamps its input and updates the `Camera`.

## 4. ANARI adapter and rewiring

- [ ] Replace `AnariMapCamera` with an `AnariCamera` in `im3e/anari/src/`. It holds the `ANARICamera` handle and a `shared_ptr<const Camera>`, and its `commitChanges()` only pushes parameters when the camera's version changed.
- [ ] In `AnariFramePipeline`:
  - Create the `Camera`, the `MapCameraController` and the `AnariCamera`.
  - `getCameraListener()` returns the controller.
  - On resize, call `getProjection().setAspectRatio(...)`.
- [ ] Change `AnariWorld::updateAsync` and `AnariHeightField::updateAsync` to take `const ViewFrustum&`.
- [ ] Add `im3e_scene` to `im3e_anari`'s link libraries.
- [ ] Check manually in the viewer that pan, tilt, heading and zoom behave as before.
- [ ] Check whether the frustum culling TODO in `im3e/anari/src/anari_height_field.cpp` still reproduces now that the frustum can't go stale. Remove the TODO if it's fixed, otherwise update it.

## 5. UI: show and edit the camera state

- [ ] `createCameraProperties()` returns a "Camera" group combining:
  - the controller's editable state;
  - the projection (FOV, near, far editable; aspect read-only);
  - the view transform (read-only).
- [ ] Check in the viewer that editing the state in the panel moves the camera, and that mouse navigation updates the panel.

## 6. Optional

- [ ] Add an integration test for `AnariCamera` against Helide, which would be the first test of `im3e_anari`.
- [ ] Derive the initial `MapCameraState` from the height map's bounds instead of a constant.
- [ ] Update `docs/architecture.md` (the `im3e_anari` section, plus an `im3e_scene` entry in the libraries table and dependency graph) and `docs/coordinate_spaces.md` (`AnariMapCamera` becomes `MapCameraController`).
