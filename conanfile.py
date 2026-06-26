from conan import ConanFile
from conan.tools.cmake import cmake_layout


class NandinaExperiment(ConanFile):
    name = "nandina_experiment"
    version = "0.1.0"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("zlib/1.3.2")
        self.requires("sdl/3.4.8")
        self.requires("thorvg/0.15.16")
        self.requires("freetype/2.14.3")
        self.requires("harfbuzz/12.3.0")
        self.requires("yoga/3.2.0")

    def build_requirements(self):
        self.tool_requires("cmake/4.3.3")
