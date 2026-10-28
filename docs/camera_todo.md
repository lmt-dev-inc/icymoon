# Camera Todo (ticket #72)

Temporary progress tracker: delete before creating the PR.

## 1. Ray fix (in `utils`)

- [x] Move `Ray` out of `im3e/utils/camera_transforms.h` into a new `im3e/utils/ray.h`, with its implementation in `src/ray.cpp`.
- [x] Fix `Ray::createViewRayFromNdc`: the near point should be at NDC z = 0, not -1, because depth is [0, 1].
- [x] `Ray::createViewRayFromNdc` should be moved to new `Camera` class and tested there
- [x] Add `test_ray.cpp` to `test_im3e_utils`:
  - NDC (0, 0) gives a ray along the view direction, with its origin on the near plane.
  - Corner NDC positions give rays through the corners of the frustum.

## 2. `im3e_scene` library

- [x] Create `im3e/scene/` with an `im3e_scene` STATIC target. It links `im3e_utils` and `im3e_utils_properties` explicitly; add it to `im3e/CMakeLists.txt`.
- [x] Move `PerspectiveProjection` and `ViewTransform` there, then delete `camera_transforms.h` and its `.cpp` from `utils`.
- [x] Add `Camera`:
  - It owns a projection and a view transform.
  - Its `ViewFrustum` is rebuilt lazily from a dirty flag, so it is never stale.
  - It has a `getVersion()` counter that increments on every change.
  - `createProperties()` exposes position, direction, up and right as read-only.
- [x] Add tests:
  - [x] `test_perspective_projection.cpp`: the matrix matches `glm::perspective`.
  - [x] `test_view_transform.cpp`: `setDirection` makes the axes orthonormal, the matrix matches `glm::lookAt`, and setters fire the change callback.
  - [x] `test_camera.cpp`: the frustum follows changes to the aspect ratio, FOV, near/far and position; the version increments on each change and stays the same when nothing changes.

## 3. `MapCameraController` (in `im3e_scene`)

- [x] Add `MapCameraController`, holding the state (target, distance, tilt, heading). The controller has no GUI dependency: it exposes navigation operations instead of event handlers:
  - `pan(ndcFrom, ndcTo)`: keeps the ground point under the cursor.
  - `rotate(headingDelta, tiltDelta)`: in radians, tilt clamped to [0, 70°].
  - `zoom(steps)`.
- [x] Move the navigation logic out of `AnariMapCamera`: `intersectHorizontalPlane`, the pose computation, and the pan, tilt, heading and zoom computations.
- [x] `AnariMapCamera` stays the `IGuiEventListener` for this story: it maps mouse events to controller operations (left-drag → pan, Ctrl+left-drag → rotate with the drag constants, wheel → zoom). Moving these bindings to the GUI layer is left for later.
- [x] Decide on zoom: exponential (`distance *= pow(0.9F, steps)`) or clamped to a min/max distance. Either way the distance must never reach zero or go negative.
- [ ] Create a story to allow setting the camera inital state through properties instead of the hard-coded target (3700, 0, 4000) and distance 5000.
- [x] Add `getProperties()` exposing the state as editable values (target, distance, tilt, heading). Setting a property clamps the value and updates the `Camera`, like the operations do.
- [ ] Add min/max values to heading so that the UI has a slider (make sure the calculated values in rotate() respect this range without being clamped)
- [ ] Add `test_map_camera_controller.cpp`, calling the operations directly:
  - **Initial pose:** tilt 0 looks straight down at the target from `distance`.
  - **Pan:** the ground point under `ndcFrom` ends up under `ndcTo`, and the target's height stays the same.
  - **Pan above the horizon:** a pan whose ray doesn't hit the plane leaves the state unchanged.
  - **Tilt:** `rotate` changes tilt, clamped to [0, 70°].
  - **Heading:** rotating by a full turn comes back to the same heading.
  - **Zoom:** one step applies the expected factor, and a large zoom keeps the distance positive.
  - **Properties:** setting a property clamps its value and updates the `Camera`, and the operations update the property values.
- [x] Bindings in `AnariMapCamera` (manual check in the viewer, see section 4): a full-width Ctrl+drag makes a full turn, and moving without the left button changes nothing.

## 4. ANARI adapter and rewiring

- [x] `AnariMapCamera` stays a single class: besides the input bindings, it holds the `ANARICamera` handle and a `shared_ptr<const Camera>`, and its `commitChanges()` only pushes parameters when the camera's version changed.
- [x] Add `im3e_scene` to `im3e_anari`'s link libraries.
- [x] Check manually in the viewer that pan, tilt, heading and zoom behave as before.
- [ ] Check whether the frustum culling TODO in `im3e/anari/src/anari_height_field.cpp` still reproduces now that the frustum can't go stale. Remove the TODO if it's fixed, otherwise update it.

## 5. UI: show and edit the camera state

- [x] `createCameraProperties()` returns a "Camera" group combining:
  - the controller's editable state;
  - the projection (FOV, near, far editable; aspect read-only);
  - the view transform (read-only).
- [x] Check in the viewer that editing the state in the panel moves the camera, and that mouse navigation updates the panel.

## 6. Documentation

- [ ] Add `docs/camera.md`: what each class is responsible for, why it is split that way, and a diagram of the camera classes and how changes flow between them (callbacks, dirty flag, version). Link it from `docs/index.md`.
- [ ] Update `docs/architecture.md`: an `im3e_scene` entry in the libraries table and dependency graph, a short `im3e_scene` section linking to `camera.md`, and the `im3e_anari` section.
- [ ] Update `docs/coordinate_spaces.md`: the navigation logic moves from `AnariMapCamera` to `MapCameraController`.

## 7. Optional

- [ ] Derive the controller's initial state from the height map's bounds instead of a constant.
