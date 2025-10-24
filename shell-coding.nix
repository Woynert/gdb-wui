#
# Nix shell used for CODING
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
mkShell {
    name = "conan-env";
    buildInputs = [
        # build tools
        
        pkg_config_wrap # first: use wrapper
        pkg-config # second: set env vars
        clang-tools # to use clandg this must come first
    ];
    shellHook = ''
        # git prompt
        source ${git}/share/git/contrib/completion/git-prompt.sh
        PS1='\[\033[0;33m\]nix:\w\[\033[0m\] $(__git_ps1 %s)\n$ '
    '';
}
