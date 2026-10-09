# Conan profiles

Each build configuration of `dev.py` (e.g. `dev.py install lnx-gcc-debug`) uses the profile of the
same name in this folder. The CMake preset and the build folder (`build/<name>`) have that name too.

## Layers

Profiles are made of four layers, one folder per layer:

| Layer       | Folder       | Defines                                                                    |
| ----------- | ------------ | -------------------------------------------------------------------------- |
| Common      | `base/`      | Settings shared by every configuration: architecture, system package manager |
| OS          | `os/`        | Target OS, and the start of the preset name (`lnx-`, `win-`)               |
| Compiler    | `compilers/` | Compiler, version, C++ standard and runtime                                |
| Variant     | `variants/`  | Build types and options (`debug`, `release`, `coverage`), and the end of the preset name |

A user-facing profile, at the root of this folder, combines one file of each layer and nothing else:

```
# lnx-gcc-debug
include(base/common)
include(os/linux)
include(compilers/gcc)
include(variants/debug)
```

Rules:
- Only user-facing profiles include layer files, so that everything a configuration is made of is
  listed in its own file.
- A layer file doesn't include files of other layers. It may extend a file of its own layer, which
  must be stated in a comment at the top: `compilers/clang-cl` extends `compilers/msvc`.
- User-facing profiles are named `<os>-<compiler>-<variant>`: the OS layer starts the preset name
  and the variant layer ends it, through `tools.cmake.cmake_layout:build_folder_vars`.

## Configurations

| Profile              | OS        | Compiler   | Variant    |
| -------------------- | --------- | ---------- | ---------- |
| `lnx-gcc-debug`      | `linux`   | `gcc`      | `debug`    |
| `lnx-gcc-release`    | `linux`   | `gcc`      | `release`  |
| `lnx-gcc-coverage`   | `linux`   | `gcc`      | `coverage` |
| `win-clang-debug`    | `windows` | `clang-cl` | `debug`    |
| `win-clang-release`  | `windows` | `clang-cl` | `release`  |
| `win-clang-coverage` | `windows` | `clang-cl` | `coverage` |

## Design notes

- **Dependencies are always built in Release**, including in `debug` and `coverage` variants where
  only our code is built in Debug (`&:build_type=Debug`). ConanCenter only provides Release binaries,
  so dependencies are downloaded instead of built from source.
- **On Windows, our code is built with clang-cl and dependencies with msvc.** Both are ABI
  compatible: the prebuilt msvc binaries of ConanCenter are used, and clang provides the same
  tooling as on Linux (source-based coverage, clang-tidy, sanitizers). The msvc version (194,
  Visual Studio 2022) is the newest one ConanCenter has binaries for.
- **On Windows, everything uses the release C runtime (`/MD`)**, including debug builds: debug and
  release runtimes can't be mixed when standard library objects cross library boundaries.
- **The OS is set explicitly**, not detected, so that `dev.py lock` resolves the dependencies of
  every OS from any OS.

## Adding a configuration

1. Add or reuse a file in each layer. For a new variant, append to `build_folder_vars` with `+=`
   to end the preset name (see `variants/coverage`).
2. Add the user-facing profile, named after the resulting preset.
3. Register it in `CONFIGS` (and optionally `GROUPS`) in `devtools/configs.py`, then run
   `dev.py lock`.
