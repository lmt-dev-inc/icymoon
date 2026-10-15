from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.build import can_run
from conan.tools.files import get, replace_in_file
import os


class AnariSdkRecipe(ConanFile):
    name = "anari"
    version = "0.14.1"
    package_type = "static-library"

    settings = "os", "compiler", "build_type", "arch"
    generators = "VirtualBuildEnv"

    requires = {
    }

    tool_requires = {
        "cmake/3.30.1",
        "ninja/1.12.1"
    }

    def source(self):
        get(self, "https://github.com/KhronosGroup/ANARI-SDK/archive/refs/tags/v0.14.1.zip", strip_root=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolChain = CMakeToolchain(self, generator="Ninja")
        toolChain.variables["BUILD_TESTING"] = "OFF"
        toolChain.variables["BUILD_VIEWER"] = "OFF"
        toolChain.variables["BUILD_EXAMPLES"] = "OFF"
        toolChain.variables["INSTALL_VIEWER_LIBRARY"] = "OFF"
        toolChain.presets_prefix = ""
        toolChain.generate()

        cmake = CMakeDeps(self)
        cmake.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        # Add anari_static target for conan to export
        self.cpp_info.components["anari_static"].set_property("cmake_target_name", "anari::anari_static")
        self.cpp_info.components["anari_static"].libs = ["anari_static"]
        self.cpp_info.components["anari_static"].defines = ["ANARI_STATIC_DEFINE"]

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindir, "example")
            self.run(cmd, env="conanrun")
