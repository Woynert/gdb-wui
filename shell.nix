#
# Nix shell used for COMPILING
#

with import <nixpkgs> {};
let

pkg_config_wrap = writeShellScriptBin "pkg-config" ''
  #!/usr/bin/env sh
  ${pkg-config-unwrapped}/bin/pkg-config $@ 2> /dev/null
  err=$?
  [ $err -ne 0 ] && exec ${pkg-config}/bin/pkg-config $@
  exit $err
'';

in
mkShell { # for GCC
#llvmPackages.stdenv.mkDerivation { # for clang
	name = "gdb-env";
	buildInputs = [
        # build tools
        
		pkg_config_wrap # first: use wrapper
		pkg-config # second: set env vars
        cmake # for building conan deps
        meson
        ninja
		bear
        
        # debug

        gdb
		gf
        valgrind
        gede
        libllvm # llvm-symbolizer
        expect  # unbuffer command, for improving tail -F

        # extra tools (for convenience)

        clang-tools
        cacert # for downloading meson modules through SSL
		git
        patchelf
        pax-utils # lddtree
		#steamPackages.steam-fhsenv-without-steam.run
        steam-run-free

        # === raylib ===

        libGL
        xorg.libX11
        xorg.libfontenc
        xorg.libICE
        xorg.libSM
        xorg.libXaw
        xorg.libXcomposite
        xorg.libXdmcp
        xorg.libxkbfile
        xorg.libXpm
        xorg.libXres
        xorg.libXScrnSaver
        xorg.libXtst
        xorg.libXv
        xorg.libxcb # might not be needed
        xorg.libXau
        xorg.xcbutil
        xorg.xcbutilwm
        xorg.xcbutilimage
        xorg.xcbutilkeysyms
        xorg.xcbutilrenderutil
        xorg.libXcursor
        xorg.libXdamage
        xorg.libXext
        xorg.libXinerama
        xorg.libXi
        xorg.libXrandr
        xorg.libXxf86vm
        xcb-util-cursor
        libossp_uuid
	];
	shellHook = ''
		# git prompt
		source ${git}/share/git/contrib/completion/git-prompt.sh
		PS1='\[\033[0;32m\]nix:\w\[\033[0m\] $(__git_ps1 %s)\n$ '
	'';
}
