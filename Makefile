.PHONY: *
default:

# ===== basic commands =====

clean:
	rm -rf deps build

cleanMeson:
	rm -rf build

conanInstall:
	conan install . --output-folder=deps --build=missing --build=editable

mesonSetupDebug:
	meson setup --reconfigure --prefix=$(CURDIR)/build --native-file deps/conan_meson_native.ini build \
		--debug -Db_ndebug=false --buildtype=debug  -Doptimization=1

compile:
	meson compile -C build

#mesonSetupRelease:
	#meson setup --reconfigure --prefix=$(CURDIR)/build --native-file deps/conan_meson_native.ini build
