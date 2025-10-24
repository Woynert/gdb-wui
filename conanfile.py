from conan import ConanFile
from conan.tools.files import copy


class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "MesonToolchain", "PkgConfigDeps"

    def requirements(self):
        self.requires("raylib/5.5")

    def configure(self):
        self.options["raylib"].shared = False
        return

    def generate(self):
        return
