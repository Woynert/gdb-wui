#!/usr/bin/env python3
import os, subprocess
# https://stackoverflow.com/a/1432949
os.chdir(os.path.dirname(os.path.abspath(__file__)))

RESET="\033[0m"; RED="\033[31m"; BLU="\033[34m"; BOLD="\033[1m"

def run_cmd(*args, **kwargs):
    print("cmd: " + " ".join(args[0]))
    result = subprocess.run(*args, **kwargs)
    return result

def fetch_dependency(directory: str, giturl: str, revision: str):
    print(f"\n{BOLD}Fetching {giturl} at {directory}.{RESET}")
    if not os.path.isdir(os.path.join(directory, ".git")):
        cmd = ["git", "clone", "--filter=blob:none", "--no-checkout", giturl, directory]
        result = run_cmd(cmd)
        if result.returncode != 0:
            print(f"{RED}Couldn't fetch {giturl}{RESET}")
            return
    run_cmd(["git", "-C", directory, "fetch"])
    run_cmd(["git", "-C", directory, "checkout", revision])

def apply_patch(directory: str, patch: str):
    patch = os.path.relpath(patch, directory)
    result = run_cmd(["git", "-C", directory, "apply", "--reverse", "--check", patch])
    if result.returncode == 0:
        print(f"{BLU}Patch applied {patch}{RESET}")
        return
    result = run_cmd(["git", "-C", directory, "apply", patch])
    if result.returncode == 0:
        print(f"{BLU}Patch applied {patch}{RESET}")
    else:
        print(f"{RED}Couldn't apply patch {patch}{RESET}")

# Dependencies:

fetch_dependency(
"cwalk",
"https://github.com/likle/cwalk",
"f45a23a13abf39d94b347d7c83810eca26a5a8d0")

fetch_dependency(
"mickstr/mickstr",
"https://github.com/Woynert/mickjc750-str",
"d20747c7ad9d7898284123987babc310c07101d7")

fetch_dependency(
"raylib",
"https://github.com/raysan5/raylib",
"808e6b9b20f76de5af1f512ae2a76af01627de74") # 6.0 unstable
# "c1ab645ca298a2801097931d1079b10ff7eb9df8") # 5.5 stable
apply_patch("raylib", "./raylib-fix-GetWindowHandler.patch")

fetch_dependency(
"raygui/raygui",
"https://github.com/raysan5/raygui",
"b9971133b2f7b7513904770d565b683a93fb3624")
