# IcyMoon

- [How to Build](#how-to-build)
  - [Prerequisites](#prerequisites)
  - [Quick Start](#quick-start)
  - [dev.py Commands](#devpy-commands)
  - [Optional Dependencies](#optional-dependencies)
    - [VisRTX](#visrtx)
    - [OpenUSD](#openusd)
  - [Compiling](#compiling)
  - [Running tests](#running-tests)
  - [Test Coverage with GCC](#test-coverage-with-gcc)
- [Development with VS Code](#development-with-vs-code)


## How to Build

The project is built with Conan and CMake. Both are installed in a local Python virtual environment (`.venv`) by `dev.py`, at the versions pinned in `requirements.txt`. You don't need to install them yourself.

### Prerequisites

The following must be installed manually:

- Python 3.10+ with the `venv` module (on Debian/Ubuntu: `sudo apt install python3-venv`)
- On Linux: GCC 11 (`gcc-11` and `g++-11`)
- On Windows: Visual Studio 2022 Build Tools with the C++ and Clang components. `dev.py setup` installs them with `winget` if they are missing (no Visual Studio IDE needed). Our code is compiled with clang-cl, while dependencies use the prebuilt MSVC binaries of ConanCenter: both are ABI compatible.
- [Vulkan SDK](https://vulkan.lunarg.com/) (tested with 1.3.290.0)

Other dependencies, including the [ANARI SDK](https://github.com/KhronosGroup/ANARI-SDK) (0.14.1), are downloaded or built by Conan.

### Quick Start

From the root folder:

```bash
python3 dev.py setup              # create .venv and install conan and cmake
source .venv/bin/activate
python3 dev.py install lnx-gcc-debug  # install dependencies and generate CMake presets
cmake --preset lnx-gcc-debug
cmake --build --preset lnx-gcc-debug
```

On Windows, activate with `.venv\Scripts\activate` and use the `win-clang-debug` configuration instead.

> On Linux, Conan may install missing system packages with `apt-get` and ask for your sudo password.

### dev.py Commands

Run `python3 dev.py -h` or `python3 dev.py <command> -h` for full help.

| Command                                | Description                                                                                                                                                                              |
| -------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `setup [--recreate]`                   | Creates `.venv`, installs the tools from `requirements.txt` and detects a default Conan profile. `--recreate` deletes `.venv` first; deactivate the virtual environment before using it. |
| `install [config\|group ...] [--all]`  | Exports the local recipes under `recipes/` and runs `conan install` for each configuration. Without arguments, installs the group of the current OS (`lnx` or `win`). `--all` installs every configuration for the current OS. `install -h` lists the configurations and groups.           |
| `lock [--update pkg ...] [--recreate]` | Creates or updates `conan.lock` for every configuration. Without options, missing entries are added and unused ones removed. `--update` upgrades the given packages. `--recreate` relocks everything. |

Available configurations, named `<os>-<compiler>-<variant>` so that names never clash between OSes:

| OS      | Configurations                                               | Group (default on this OS) |
| ------- | ------------------------------------------------------------ | -------------------------- |
| Linux   | `lnx-gcc-debug`, `lnx-gcc-release`, `lnx-gcc-coverage`       | `lnx`                      |
| Windows | `win-clang-debug`, `win-clang-release`, `win-clang-coverage` | `win`                      |

Each configuration maps to a profile under `profiles/` and produces a CMake preset with the same name. Debug configurations only build our code in Debug: dependencies stay in Release, as ConanCenter only provides Release binaries. See [profiles/README.md](profiles/README.md) for how profiles are organized.

`conan.lock` pins the exact version and revision of every dependency, and `dev.py install` applies it automatically. Run `dev.py lock` after changing requirements in `conanfile.py` or a recipe under `recipes/`, then review and commit the `conan.lock` diff.

To add a configuration, follow the steps in [profiles/README.md](profiles/README.md#adding-a-configuration).

### Optional Dependencies

#### VisRTX

Clone the [VisRTX](https://github.com/NVIDIA/VisRTX) GitHub repo and checkout the right version:
```bash
git clone https://github.com/NVIDIA/VisRTX.git
git checkout v0.12.0
```

Dependencies to install:
- [NVIDIA CUDA Toolkit](https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=22.04&target_type=deb_local)
- [OptiX](https://developer.nvidia.com/designworks/optix/download)

Build and install using CMake:
```bash
cd /path/to/visrtx
mkdir build
cd build
cmake ..
make
make install
```

#### OpenUSD

[OpenUSD](https://github.com/PixarAnimationStudios/OpenUSD) (tested with 24.11) is not required by the project, so `dev.py install` doesn't build it. To add it to your local Conan cache, activate the virtual environment and run from the root folder:

```bash
conan create ./recipes/open_usd --build=missing
```

To use tools such as usdview, source the Conan build environment of your configuration. It appends the following environment variables:

| Environment Variable | Value to add                         |
| -------------------- | ------------------------------------ |
| `PYTHONPATH`         | `<path_to_OpenUSD>/build/lib/python` |
| `PATH`               | `<path_to_OpenUSD>/build/bin`        |

```bash
source build/<my_preset>/generators/conanbuild.sh
```

### Compiling

`dev.py install` creates a `CMakeUserPresets.json` file in the root folder with one preset per installed configuration. List them with:

```bash
cmake --list-presets

Available configure presets:

  "lnx-gcc-debug"   - 'lnx-gcc-debug' config
  "lnx-gcc-release" - 'lnx-gcc-release' config
```

With the virtual environment active, configure and build with a chosen preset from the root folder:

```bash
cmake --preset <my_preset>
cmake --build --preset <my_preset>
```

### Running tests

The project uses `ctest` to handle tests and supports presets. To get a list of ctest presets, run the following command from the root folder:
```bash
ctest --list-presets

Available test presets:

  "lnx-gcc-debug"
  "lnx-gcc-release"
```

All tests can be run with the `--preset` argument:
```bash
ctest --preset lnx-gcc-debug
```

Each test is labelled as a `"unit_test"` or an `"integration_test"`. This allows to filter out tests by type:
```bash
ctest --preset lnx-gcc-debug -L "unit_test" # to run unit tests only
ctest --preset lnx-gcc-debug -L "integration_test" # to run integration tests only
```

### Test Coverage with GCC

Test coverage is supported with lcov via the `lnx-gcc-coverage` configuration:
```bash
python3 dev.py install lnx-gcc-coverage
cmake --preset lnx-gcc-coverage
cmake --build --preset lnx-gcc-coverage --target im3e_test_coverage
```

There are several CMake targets that generate different test coverage reports:
- `im3e_test_coverage`: combines the coverage of all tests
- `im3e_unit_test_coverage`: generate a report for unit tests only (excluding integration tests)


## Development with VS Code

The repository includes a shared VS Code setup under `.vscode/`:

- `extensions.json` recommends CMake Tools, C/C++ and Python.
- `settings.json` makes CMake Tools use the Conan-generated presets, which also tell it which CMake binary to use. VS Code picks up `.venv` automatically and activates it in new terminals. Run `python3 dev.py setup` and `python3 dev.py install` before opening the folder.
- It also sets shared editor conventions: format on save and a ruler at 120 columns.
- `launch.json` provides a gdb "Launch" configuration for the target selected in CMake Tools.
