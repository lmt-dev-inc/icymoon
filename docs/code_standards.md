# Code Standards

Rules for the C++ code (`im3e/`, `applications/`) and CMake scripts (`cmake/`) of IcyMoon. Third-party code under `external/` is excluded.

The project follows the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). This document records the project's own choices and additions on top of them; where the two differ, this document takes precedence. Rules refer to guidelines by number where relevant, e.g. [ES.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-always).

Each rule says whether it is checked automatically today. Rules without automation are enforced in code review; [Enforcement](#enforcement) explores how to automate them later.

- [Formatting](#formatting)
- [Compiler warnings](#compiler-warnings)
- [Namespaces](#namespaces)
- [Naming](#naming)
- [Functions](#functions)
- [Initialization](#initialization)
- [Library folders](#library-folders)
- [Files and includes](#files-and-includes)
- [Error handling](#error-handling)
- [Text encoding and paths](#text-encoding-and-paths)
- [Library code](#library-code)
- [Tests](#tests)
- [CMake scripts](#cmake-scripts)
- [Enforcement](#enforcement)

## Formatting

- Code is formatted with clang-format, using `.clang-format` at the root of the repository: 4-space indentation, 120-column limit, braces on their own line for classes, functions and control statements.
- VS Code formats on save (`.vscode/settings.json`).

*Automated:* in the editor only. Nothing checks it in CI yet.

## Compiler warnings

- Warnings are errors: `/W4 /WX` with msvc and clang-cl, `-Wall -Wextra -pedantic -Werror` with gcc (root `CMakeLists.txt`).
- Fix the code rather than disabling a warning. When a warning must be disabled, do it in `CMakeLists.txt` with a comment explaining why (see `-Wno-deprecated` for OpenUSD).
- Disabled on purpose: `-Wmissing-field-initializers` with gcc, and `-Wmissing-designated-field-initializers` with clang, so that designated initializers can omit fields (see [Initialization](#initialization)). The clang option requires clang 19 or later.

*Automated:* yes, by the compilers.

## Namespaces

- **Never use `using namespace std;`, or `using namespace` with any namespace inside `std`** (`std::filesystem`, `std::chrono`...). Always write the `std::` prefix.
- The only exceptions are the literal namespaces, which are needed for literal suffixes that can't take a prefix: `std::literals`, `std::string_literals`, `std::string_view_literals`, `std::chrono_literals`.

  ```cpp
  using namespace std::string_view_literals;  // OK: enables "Root"sv
  using namespace std::chrono_literals;       // OK: enables 100ms
  using namespace std::filesystem;            // Not allowed: write std::filesystem::path
  ```
- Code lives in the `im3e` namespace. Sub-namespaces group related free functions, e.g. `im3e::path` (`im3e/utils/core/path_utils.h`).
- `.cpp` files may use `using namespace im3e;`. Headers never use `using namespace`.
- Implementation details of a `.cpp` file go in an anonymous namespace.
- Calls to standard functions are always qualified, including those that would compile without the prefix through argument-dependent lookup (`std::move`, `std::pow`, `std::back_inserter`...). Without the prefix, a call can silently resolve to another function: unqualified `pow(float, float)` calls the C function `::pow(double, double)`.

*Automated:* no.

## Naming

| Element | Convention | Example |
|---|---|---|
| Files | `snake_case` | `vulkan_command_queue.cpp` |
| Classes, structs, enums | `PascalCase` | `VulkanCommandQueue` |
| Interfaces (abstract classes) | `I` + `PascalCase` | `ICommandQueue` |
| Functions and methods | `camelCase` | `startScopedCommand` |
| Private methods | `_` + `camelCase` | `_releaseCompletedCommands` |
| Enum values | `PascalCase` | `CommandExecutionType::Async` |
| Constants (`constexpr`, `const` at namespace scope) | `PascalCase` | `DefaultFrameInFlightCount` |
| Member variables | `m_` + name | `m_name` |
| Pointers (raw and smart) | `p` prefix | `pDevice`, `m_pVkCommandPool` |
| References | `r` prefix | `const IDevice& rDevice`, `m_rDevice` |
| Vulkan handles and structs | `vk` prefix | `vkDevice`, `vkCreateInfo` |

The prefixes combine: a member pointer is `m_p...`, a member reference is `m_r...`.

The leading `_` marks methods that only the class itself can call. It applies to `private` methods only: `protected` methods are an interface for derived classes and are named like public ones. The `_` is always followed by a lowercase letter: names starting with `_` + uppercase letter, or containing `__`, are reserved by the C++ standard.

*Automated:* no.

## Functions

- Functions that return a value use a trailing return type. Functions that return nothing are written `void`.

  ```cpp
  auto getVkCommandBuffer() const -> VkCommandBuffer;
  void waitIdle();
  ```
- Don't write `this->` to access members: the `m_` and `_` prefixes already distinguish them. Use it only where C++ requires it, to access a member of a base class that depends on a template parameter.

  ```cpp
  _releaseCompletedCommands();                     // not this->_releaseCompletedCommands();
  auto pThis = shared_from_this();                 // not this->shared_from_this();
  [this](auto w, auto h) { _onWindowResized(w, h); }  // members are accessible through the [this] capture
  ```
- Free functions declared in a header are defined with their qualified name: `auto im3e::getCurrentExecutableFolder() -> std::filesystem::path { ... }`.
- Public APIs are documented with `///` comments (`/// @brief ...`).

*Automated:* no.

## Initialization

- Initialize every object ([ES.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-always)). Use braces: `VkFence vkFence{};`.
- Designated initializers may omit fields. Omitted fields take their default member initializer, or are value-initialized (zero for numbers, pointers and enums, default constructor for classes). This keeps initializations of large C structs, like Vulkan's, to the fields that matter:

  ```cpp
  VkCommandPoolCreateInfo vkCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndex,
  };  // pNext is nullptr
  ```
- Give the members of our own structs a default member initializer when zero isn't the right default, so that the default is stated once, at the definition ([C.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-in-class-initializer)): `uint32_t frameInFlightCount{1U};`.

*Automated:* partially. The compilers warn about uninitialized variables they can detect.

## Library folders

### Targets and groups

Every folder under `im3e/` is either a **target** folder or a **group** folder, never both. A folder that is both makes it unclear which files belong to which target, and lets the folder tree contradict the dependencies, e.g. when a target depends on another target nested in its own folder.

- A **target folder** defines one CMake target in its `CMakeLists.txt`, and contains only the parts of that target:
  - `include/`: public headers, for targets that other targets link;
  - `src/`: implementation and private headers;
  - `data/`: files the target needs at run time, e.g. test data;
  - for executables, such as tests, the `.cpp` files directly in the folder, as nothing is public.
- A **group folder** contains only other folders, and a `CMakeLists.txt` that adds them with `add_subdirectory`. It has no code and no target.
- Folder names are singular and `snake_case`.

### Libraries

A library is a group: it keeps the library target together with the targets that exist for it, its mocks and its tests, because a change to a library usually changes them too.

```
im3e/scene/                                    group
  CMakeLists.txt                               adds lib/, and mock/ and test/ when tests are built
  lib/                                         im3e_scene
    CMakeLists.txt
    include/im3e/scene/camera.h
    include/im3e/scene.h                       umbrella header
    src/camera.cpp
  mock/                                        mock_im3e_scene
    CMakeLists.txt
    include/im3e/scene/mock_camera.h
    src/mock_camera.cpp
  test/                                        group
    CMakeLists.txt
    unit/                                      test_im3e_scene
      CMakeLists.txt
      test_camera.cpp
    integration/                               integration_im3e_scene
      CMakeLists.txt
      integration_camera.cpp
      data/
```

- **Names come from paths.** The library target is `im3e_` followed by the library's path below `im3e/`, with `/` replaced by `_`, and its headers are included through that path: `im3e/render/anari/` is `im3e_render_anari`, and its headers are `<im3e/render/anari/...>`. The other targets are prefixed with their type: `mock_im3e_render_anari`, `test_im3e_render_anari` (unit tests), `integration_im3e_render_anari`. Sorted alphabetically, targets of the same type then sit together, in `bin/` as in `target_link_libraries`, where linked mocks are visible at a glance. Mock headers are included through the library's path, with the same prefix as their target: `<im3e/render/anari/mock_anari_renderer.h>`.
- **Each test type is its own target**, in its own folder under `test/`, with its own `ctest` label (see [Tests](#tests)). Adding a test type means adding a folder. Helpers shared by several test types of a library go in a `test/common/` target, `test_common_im3e_<path>`.
- **Mocks only implement interfaces** from the library's public headers. Never replace a concrete class at link time (a "link seam"): it depends on link order and ODR (one-definition rule) details, breaks silently, and behaves differently between linkers. Mocks define their constructor and destructor in `src/`, so that the mock methods are compiled once rather than in every test, as the [gMock cookbook](https://google.github.io/googletest/gmock_cook_book.html#making-the-compilation-faster) recommends.

### Families of libraries

Several implementations of the same interfaces form a group, each in its own library, so that they sort together and a consumer only links the backend it uses. The library holding what they share is named `api` when it only holds interfaces, and after its role otherwise.

```
im3e/
  scene/        library  im3e_scene
  render/       group
    api/        library  im3e_render_api     renderer-neutral interfaces
    anari/      library  im3e_render_anari
    vulkan/     library  im3e_render_vulkan
  gui/          group
    imgui/      library  im3e_gui_imgui      workspace and panels
    glfw/       library  im3e_gui_glfw
```

### Include directories

Each target only exposes its own `include/` folder, so a target can only include the headers of the targets it links. The repository root is not an include directory.

| Target | Public include directory | Private include directory |
|---|---|---|
| Library | `lib/include/` | `lib/src/` |
| Mock | `mock/include/` | `mock/src/` |
| Test executable | | `lib/src/`, only when the tests need the library's internal classes |

### Public headers

- One header per public concept, named after its main type: `camera.h` declares `Camera`. A small type that only exists to configure another one can share its header, e.g. `MapCameraState` in `map_camera_controller.h`.
- Each library has an **umbrella header** that includes all its public headers and contains nothing else, so that an application includes one file per library. It is one level above the library's headers, so that it can't clash with a class header: `<im3e/scene.h>` includes `<im3e/scene/camera.h>`..., and `<im3e/render/anari.h>` includes `<im3e/render/anari/...>`.
- Code under `im3e/` never includes an umbrella header: it includes the headers it uses (see [Files and includes](#files-and-includes)), so that a change to a header only rebuilds the files that use it. Umbrella headers are for applications and other users of the library.

*Automated:* no. Once every library follows this layout, the build enforces the include directories.

## Files and includes

- Headers use `#pragma once`.
- Includes are grouped, with a blank line between groups:
  1. the header of the current `.cpp` file,
  2. private headers of the same target, from its `src/` folder (`"..."`),
  3. IcyMoon public headers (`<im3e/...>`), including those of the same library: a public header is always included through its full path, never with `"..."`,
  4. third-party headers (`<fmt/format.h>`, `<imgui.h>`...),
  5. standard library headers (`<filesystem>`, `<string>`...).
- Include what you use: every file includes the headers of everything it uses, even when another header already happens to include them. Standard libraries differ in which headers they include internally, so code relying on indirect includes compiles with one standard library and fails with another (libstdc++ on Linux vs Microsoft's on Windows).

*Automated:* no.

## Error handling

- Errors are reported with exceptions, through the helpers of `im3e/utils/core/throw_utils.h`: `throwIfNull`, `throwIfArgNull`, `throwIfFalse`, and `throwIfVkFailed` for Vulkan results.
- Constructors validate their dependencies with `throwIfArgNull`, e.g. `m_pDevice(throwIfArgNull(std::move(pDevice), "... requires a device"))`.

*Automated:* no.

## Text encoding and paths

- Text is UTF-8 in `std::string` / `std::string_view`. Use `std::wstring` only to call Windows APIs that require it.
- `std::filesystem::path` is UTF-16 on Windows: its `c_str()` returns `const wchar_t*`. To pass a path to a library that takes UTF-8 strings (GDAL, ImGui...), or to build a path from one, use `path::toUtf8()` / `path::fromUtf8()` from `im3e/utils/core/path_utils.h`.
- Don't use `std::filesystem::path::string()` to pass a path to a library: on Windows it converts to the process code page, which isn't UTF-8 and can't represent every character.
- To format a path, pass it to `fmt::format` directly and include `<fmt/std.h>`. Use `{:g}` for `/` separators on every OS.

*Automated:* no.

## Library code

IcyMoon is meant to be used as a library from other languages, Python first. Its code then runs inside someone else's process.

- Don't rely on process-wide settings that only our own executables would have: Windows application manifests, `setlocale`, the Windows code page.
- Don't change process-wide state from library code.

*Automated:* no.

## Tests

- Tests use GoogleTest and GoogleMock (`im3e/test_utils/test_utils.h`).
- Each test type is a target in its own folder of the library's `test/` group (see [Libraries](#libraries)), and its files are prefixed with the type, so that they are easy to tell apart when navigating: unit tests are in `test/unit/test_<name>.cpp`, integration tests in `test/integration/integration_<name>.cpp`. They are labelled `unit_test` and `integration_test` for `ctest -L`.
- Test suites are named after what they test with a `Test` suffix: `TEST(PathUtilsTest, toUtf8)`.
- Checks use `EXPECT_THAT` / `ASSERT_THAT` with matchers, not the comparison macros (`EXPECT_EQ`, `EXPECT_NE`, `EXPECT_TRUE`, `EXPECT_LT`...):

  ```cpp
  EXPECT_THAT(path::toUtf8(filePath), Eq(Utf8Path));  // not EXPECT_EQ(path::toUtf8(filePath), Utf8Path)
  EXPECT_THAT(std::filesystem::is_directory(folder), IsTrue());
  ASSERT_THAT(pLoader, NotNull());
  ```

  GoogleTest recommends this style. From its [FAQ](https://google.github.io/googletest/faq.html) ("Why does GoogleTest support `EXPECT_EQ(NULL, ptr)` and `ASSERT_EQ(NULL, ptr)` but not `EXPECT_NE(NULL, ptr)` and `ASSERT_NE(NULL, ptr)`?"):

  > with the growth of the gMock matcher library, we are encouraging people to use the unified `EXPECT_THAT(value, matcher)` syntax more often in tests. One significant advantage of the matcher approach is that matchers can be easily combined to form new matchers, while the `EXPECT_NE`, etc, macros cannot be easily combined.

  The comparison macros aren't deprecated, but matchers also give one form for every check and detailed failure messages. Common matchers are imported into `im3e` by `im3e/test_utils/test_utils.h`; add new ones there.
  References: [`EXPECT_THAT`](https://google.github.io/googletest/reference/assertions.html#EXPECT_THAT), [matchers reference](https://google.github.io/googletest/reference/matchers.html).
- Exceptions are checked with `EXPECT_THROW` / `EXPECT_NO_THROW`.

*Automated:* the labels, through `im3e_add_unit_tests_executable` and `im3e_add_integration_tests_executable` (`cmake/im3e_tests.cmake`).

## CMake scripts

CMake has no private functions: once a file is included, everything it defines can be called from anywhere. Like C++ modules, which keep public headers at the module root and implementation in `src/`, scripts make what is public visible from their location and name:

- `.cmake` files at the root of a folder are public: any `CMakeLists.txt` may include them and call their functions.
- Files in a `src/` subfolder are internal: only the scripts of the parent folder use them.
- Internal functions and macros are prefixed with `im3e_internal_`, and are defined in `src/`. The prefix shows misuse where the function is called. Don't use a leading `_` instead: when a command is redefined, CMake keeps the previous one under the name prefixed with `_`, which can clash.
- Scripts that are run rather than included (`cmake -P`, or loaded by CTest through `TEST_INCLUDE_FILES`) are internal and named with a `_script` suffix. Their header comment says what runs them and with which variables.
- Public scripts include internal ones relative to their own location: `include("${CMAKE_CURRENT_LIST_DIR}/src/<name>.cmake")`. Paths to files used later, e.g. a script run by a target, use `CMAKE_CURRENT_FUNCTION_LIST_DIR` from within the function.

```
cmake/code_coverage/
  im3e_init_code_coverage.cmake                public: im3e_init_code_coverage()...
  src/
    im3e_llvm_code_coverage.cmake              internal: im3e_internal_init_llvm_code_coverage()
    im3e_llvm_coverage_report_script.cmake     internal: run by the im3e_coverage_report target
```

*Automated:* no.

## Enforcement

Nothing below is set up yet. This section lists the options for automating the rules, to choose from later.

### Where checks could run

- **`dev.py lint`**: a new command running all checks locally, like `dev.py install`.
- **CI**: a job in `.github/workflows/cmake-multi-platform.yml` running `dev.py lint`, so that pull requests can't break the rules.
- **Editor**: VS Code runs clang-format on save already, and can show clang-tidy findings while editing (C/C++ extension).
- **Pre-commit hook**: checks before each commit. Optional, as it slows down commits.

### Tools per rule

| Rule | Tool | Notes |
|---|---|---|
| Formatting | `clang-format --dry-run --Werror` | Straightforward. Pin the clang-format version (currently 19, bundled with the Visual Studio Build Tools), as formatting differs between versions. |
| No `using namespace std` | Script (regular expression) in `dev.py lint` | clang-tidy's `google-build-using-namespace` allows only literal namespaces, but it would also reject `using namespace im3e;`, which `.cpp` files use. A small script matches the rule exactly. |
| No `using namespace` in headers | clang-tidy `google-global-names-in-headers` | |
| Qualified standard calls | clang's `-Wunqualified-std-cast-call` (already on, `std::move`/`std::forward` only) | No tool covers all functions. Code review. |
| Naming | clang-tidy `readability-identifier-naming` | Covers case styles and the `m_` prefix. Pointer prefixes are only partially supported (parameters, not members), reference and `vk` prefixes not at all. |
| Leading `_` on private methods | clang-tidy `readability-identifier-naming` (`PrivateMethodPrefix: _`) | Exact match for the rule. |
| `EXPECT_THAT` instead of comparison macros | Script (regular expression) in `dev.py lint` | Flag `EXPECT_`/`ASSERT_` + `EQ`, `NE`, `TRUE`, `FALSE`, `LT`, `LE`, `GT`, `GE`, `STREQ`... in test files. |
| No `this->` unless required | Script (regular expression) in `dev.py lint` | No clang-tidy check removes it. A script can flag every `this->` outside templates for review. |
| Trailing return types | clang-tidy `modernize-use-trailing-return-type` | Also rewrites `void` functions, unlike our convention. Either adopt `auto f() -> void` or skip this check. |
| Library folders | Script in `dev.py lint` | Walk `im3e/`: a folder that defines a target may only contain `CMakeLists.txt`, `include/`, `src/`, `data/` and, for executables, `.cpp` files; any other folder may only contain folders and `CMakeLists.txt`. Can also check target names and include paths against folder paths. |
| No umbrella headers inside `im3e/` | Script in `dev.py lint` | Flag includes of `<im3e/X.h>` where `im3e/X/` is a library. |
| Include what you use | clang-tidy `misc-include-cleaner` | Would have caught the missing includes found when porting to Windows. Can report many findings at first. |
| Include order | clang-format `IncludeCategories` | Partially configured in `.clang-format`; would need to match the groups above. |
| Paths and encoding | Script flagging `.string().c_str()` and `.c_str()` on paths, or code review | Hard to check reliably without type information. |
| Library code | Code review | |

clang-tidy needs the compile commands of a configured build, which CMake writes to `build/<preset>/compile_commands.json`. Running it only on changed files keeps it fast enough for CI.
