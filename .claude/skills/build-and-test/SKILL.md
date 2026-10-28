---
name: build-and-test
description: How to configure, build and run the tests of IcyMoon (C++/CMake/Conan) on Windows and Linux. Use when asked to build the project, compile a target, run unit or integration tests, or produce a coverage report, and to validate code changes.
allowed-tools: PowerShell(.venv\Scripts\Activate.ps1), PowerShell(cmake --preset *), PowerShell(cmake --build --preset *), PowerShell(ctest --preset *), PowerShell(cmake --list-presets*), Bash(source .venv/bin/activate), Bash(cmake --preset *), Bash(cmake --build --preset *), Bash(ctest --preset *), Bash(cmake --list-presets*)
---

# Build and test IcyMoon

The build procedure is documented in the "How to Build" section of [README.md](../../../README.md): setup, `dev.py` commands, presets, compiling, running tests and coverage. Read the section you need there; this skill only adds what the README doesn't say.

## Rules

- Activate `.venv` in the same command as every cmake/ctest call, because shell state doesn't persist between commands: `.venv\Scripts\Activate.ps1; cmake --build --preset <name>` in PowerShell, `source .venv/bin/activate && cmake --build --preset <name>` in Bash. Don't call `.venv\Scripts\cmake.exe` directly: the environment may need more than the cmake binary. Each part of these commands matches this skill's `allowed-tools`, so they run without a permission prompt. Stick to these commands; don't explore with other commands to work out how to build.
- Always go through presets (`--preset <name>`) for configure, build and test. **Never** run `cmake build/<dir>` or `cmake --build build/<dir>`: without the preset, CMake loses the Visual Studio environment and the Conan toolchain, wipes the cache, and later configures fail with "could not find anari".
- Run every command from the repository root.
- Pick the debug preset of the current OS unless told otherwise. `cmake --list-presets` shows the installed ones.

## Iterating faster

- Build one target with `cmake --build --preset <name> --target <target>`. Test executable names come from the `TARGET` argument of `im3e_add_unit_tests_executable` / `im3e_add_integration_tests_executable` in the library's `test/*/CMakeLists.txt`.
- Select tests with `ctest --preset <name> -R <regex>`, and add `--output-on-failure` to see why a test failed.
- ctest doesn't rebuild, and GTest discovery happens at build time: build before running ctest.

## Troubleshooting

- **Configure fails with a Conan package not found (e.g. anari):** check that `build/<preset>/CMakeFiles/<cmake version>/CMakeSystem.cmake` includes `conan_toolchain.cmake`. If it doesn't, reconfigure with `cmake --preset <name> --fresh`.
- **`.venv` doesn't exist or `cmake` isn't found after activation:** run `python dev.py setup` (`python3` on Linux) and ask the user before doing so. The preset's own CMake path is `cmakeExecutable` in `build/<preset>/generators/CMakePresets.json`.
- **A test executable fails to start on Windows (missing DLL):** the DLLs are copied next to the executable at build time by `im3e_copy_runtime_dlls` ([cmake/im3e_runtime_dlls.cmake](../../../cmake/im3e_runtime_dlls.cmake)); rebuild the target.
