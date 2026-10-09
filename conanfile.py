from conan import ConanFile
from conan.errors import ConanException
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.env import Environment, VirtualBuildEnv
from conan.tools.microsoft import VCVars
import os
import subprocess


class IcyMoonEngineRecipe(ConanFile):
    name = "icymoon_engine"
    version = "0.1"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"

    requires = {
        "anari/0.14.1",
        "cimg/3.3.2",
        "fmt/11.0.2",
        "gdal/3.10.3",
        "glfw/3.4",
        "glm/1.0.1",
        "proj/9.3.1",  # required by GDAL
        "vulkan-headers/1.3.243.0",  # version depended on by vulkan-memory-allocator
        "vulkan-memory-allocator/cci.20231120",
        "whereami/cci.20220112",
    }

    test_requires = {
        "gtest/1.15.0",
    }

    options = {
        "coverage": [None, "on"],
    }

    default_options = {
        # Fix for build errors with boost 1.91.0, required by GDAL through arrow:
        "boost/*:without_cobalt": True,
        "coverage": None,
        "cimg/*:enable_fftw": False,
        "cimg/*:enable_jpeg": True,
        "cimg/*:enable_openexr": False,
        "cimg/*:enable_png": True,
        "cimg/*:enable_tiff": True,
        "cimg/*:enable_ffmpeg": False,
        "cimg/*:enable_opencv": False,
        "cimg/*:enable_magick": False,
        "cimg/*:enable_xrandr": False,
        "cimg/*:enable_xshm": False,
        "gdal/*:shared": True,
        "gdal/*:with_curl": True,
    }

    tool_requires = {
        "cmake/3.30.1",
        "ninja/1.12.1",
    }

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.presets_prefix = ""

        # Conan replaces the `options` dict with an Options object at runtime, which Pylance can't see
        if self.options.coverage == "on": # pyright: ignore[reportAttributeAccessIssue]
            toolchain.cache_variables["TEST_COVERAGE"] = True

        proj_res_path = os.path.join(self.dependencies["proj"].package_folder, "res")
        toolchain.cache_variables["PROJ_RES_PATH"] = proj_res_path

        if self.settings.os == "Windows":
            toolchain.presets_build_environment = self._build_environment_with_vcvars()

        toolchain.generate()

        cmake = CMakeDeps(self)
        cmake.generate()

    def _build_environment_with_vcvars(self) -> Environment:
        """Build environment of the CMake presets, including the Visual Studio environment.

        clang-cl and msvc need the environment set by Visual Studio's vcvarsall.bat (PATH, INCLUDE,
        LIB...). Conan only applies it to its own commands, through conanvcvars.bat. Adding it to the
        presets lets `cmake --preset` and VS Code work from any terminal, without activating it first.
        """
        env = VirtualBuildEnv(self).environment()

        # Generates conanvcvars.bat, which calls vcvarsall.bat for the VS version and toolset of the profile
        VCVars(self).generate()
        vcvars = os.path.join(self.generators_folder, "conanvcvars.bat")
        result = subprocess.run(f'cmd /c "call "{vcvars}" >nul && set"', capture_output=True, text=True)
        if result.returncode != 0:
            raise ConanException(f"Failed to run {vcvars}:\n{result.stdout}{result.stderr}")

        # Keep the variables that vcvarsall.bat added or changed, except the ones only meaningful to
        # an interactive prompt (__VSCMD_PREINIT_PATH is a copy of the PATH at install time)
        ignored = {"PROMPT", "VSCMD_START_DIR", "__VSCMD_PREINIT_PATH"}
        before = {name.upper(): value for name, value in os.environ.items()}
        for line in result.stdout.splitlines():
            name, sep, value = line.partition("=")
            if not sep or name.upper() in ignored or before.get(name.upper()) == value:
                continue
            if name.upper() == "PATH":
                old_entries = set(before.get("PATH", "").split(os.pathsep))
                env.prepend_path("PATH", [entry for entry in value.split(os.pathsep)
                                          if entry and entry not in old_entries])
            else:
                env.define(name, value)
        return env

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
