.PHONY: *
default:

# ===== basic commands =====

clean:
	rm -rf build

mesonSetupDebug:
	meson setup --reconfigure --prefix=$(CURDIR)/build build \
		--debug -Db_ndebug=false --buildtype=debug  -Doptimization=1

compile: generate
	meson compile -C build

generate: src/gdb_woy_api.h
src/gdb_woy_api.h: src/gdb_woy_api.py
	src/bin2header_struct.py -i src/gdb_woy_api.py -o src/gdb_woy_api.h -v PYTHON_CODE

run:
	LSAN_OPTIONS=suppressions=suppr.txt ./build/gdbwui
