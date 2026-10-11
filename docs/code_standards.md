# Code Standards

Rules for C++ code in IcyMoon (`im3e/`, `applications/`). Third-party code under `external/` is excluded.

Each rule says whether it is checked automatically today. Rules without automation are enforced in code review; [Enforcement](#enforcement) explores how to automate them later.

- [Formatting](#formatting)
- [Compiler warnings](#compiler-warnings)
- [Namespaces](#namespaces)
- [Naming](#naming)
- [Functions](#functions)
- [Files and includes](#files-and-includes)
- [Error handling](#error-handling)
- [Text encoding and paths](#text-encoding-and-paths)
- [Library code](#library-code)
- [Tests](#tests)
- [Enforcement](#enforcement)

## Formatting

- Code is formatted with clang-format, using `.clang-format` at the root of the repository: 4-space indentation, 120-column limit, braces on their own line for classes, functions and control statements.
- VS Code formats on save (`.vscode/settings.json`).

*Automated:* in the editor only. Nothing checks it in CI yet.

## Compiler warnings

- Warnings are errors: `/W4 /WX` with msvc and clang-cl, `-Wall -Wextra -pedantic -Werror` with gcc (root `CMakeLists.txt`).
- Fix the code rather than disabling a warning. When a warning must be disabled, do it in `CMakeLists.txt` with a comment explaining why (see `-Wno-deprecated` for OpenUSD).

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

## Files and includes

- Headers use `#pragma once`.
- Includes are grouped, with a blank line between groups:
  1. the header of the current `.cpp` file,
  2. other headers of the same module (`"..."`),
  3. other IcyMoon headers (`<im3e/...>`),
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
- Unit tests are in `test/test_<name>.cpp`, integration tests in `test/integration/integration_<name>.cpp`. They are labelled `unit_test` and `integration_test` for `ctest -L`.
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
| Include what you use | clang-tidy `misc-include-cleaner` | Would have caught the missing includes found when porting to Windows. Can report many findings at first. |
| Include order | clang-format `IncludeCategories` | Partially configured in `.clang-format`; would need to match the groups above. |
| Paths and encoding | Script flagging `.string().c_str()` and `.c_str()` on paths, or code review | Hard to check reliably without type information. |
| Library code | Code review | |

clang-tidy needs the compile commands of a configured build, which CMake writes to `build/<preset>/compile_commands.json`. Running it only on changed files keeps it fast enough for CI.
