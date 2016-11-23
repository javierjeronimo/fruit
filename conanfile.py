from conans import ConanFile, CMake


class FruitConan(ConanFile):
    name = "Fruit"
    version = "2.0.4"
    url = "https://github.com/javierjeronimo/fruit"
    license = "see https://github.com/javierjeronimo/fruit/blob/master/COPYING"
    generators = "cmake"
    settings = "os", "compiler", "build_type", "arch"

    requires = (("Boost/1.60.0@lasote/stable"))

    # No exports necessary

    def source(self):
        # this will create a hello subfolder, take it into account
        self.run("git clone https://github.com/javierjeronimo/fruit.git")

    def build(self):
        cmake = CMake(self.settings)
        self.run('cmake %s/fruit %s' % (self.conanfile_directory, cmake.command_line))
        self.run("cmake --build . %s" % cmake.build_config)
        self.run("make -j")

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy(pattern="*.a", dst="lib", src="src", keep_path=False)
        self.copy(pattern="*.lib", dst="lib", src="src", keep_path=False)
        self.copy(pattern="*.dll", dst="bin", src="src", keep_path=False)
        self.copy(pattern="*.so*", dst="lib", src="src", keep_path=False)
        self.copy(pattern="*.dylib*", dst="lib", src="src", keep_path=False)
