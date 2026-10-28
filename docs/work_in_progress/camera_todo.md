# Camera Todo (ticket #72)

Temporary progress tracker: delete before creating the PR.

## 4. ANARI adapter and rewiring

- [x] `AnariMapCamera` stays a single class: besides the input bindings, it holds the `ANARICamera` handle and a `shared_ptr<const Camera>`, and its `commitChanges()` only pushes parameters when the camera's version changed.
- [x] Add `im3e_scene` to `im3e_anari`'s link libraries.
- [x] Check manually in the viewer that pan, tilt, heading and zoom behave as before.
- [ ] Check whether the frustum culling TODO in `im3e/anari/src/anari_height_field.cpp` still reproduces now that the frustum can't go stale. Remove the TODO if it's fixed, otherwise update it.


## 7. Optional

- [ ] Derive the controller's initial state from the height map's bounds instead of a constant.
