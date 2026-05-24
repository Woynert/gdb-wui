#!/usr/bin/env bash
set -Eeuo pipefail
set -x

# Source - https://stackoverflow.com/a/246128
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

fetch_dependency ()
{
    DIRECTORY="$1"
    URL="$2"
    REVISION="$3"

    printf '\033[0;31m'"\nCLONING $URL ($REVISION)..."'\033[0m\n'
    cd "$SCRIPT_DIR"
    git clone --filter=blob:none --no-checkout "$URL" "$DIRECTORY"
    cd "$DIRECTORY"
    git checkout "$REVISION"
}

DIRECTORY="mickstr/mickstr"
URL="https://github.com/Woynert/mickjc750-str"
REVISION="d20747c7ad9d7898284123987babc310c07101d7"
fetch_dependency "$DIRECTORY" "$URL" "$REVISION"

DIRECTORY="cwalk"
URL="https://github.com/likle/cwalk"
REVISION="f45a23a13abf39d94b347d7c83810eca26a5a8d0"
fetch_dependency "$DIRECTORY" "$URL" "$REVISION"

DIRECTORY="raylib"
URL="https://github.com/raysan5/raylib"
REVISION="c1ab645ca298a2801097931d1079b10ff7eb9df8"
fetch_dependency "$DIRECTORY" "$URL" "$REVISION"

DIRECTORY="raygui/raygui"
URL="https://github.com/raysan5/raygui"
REVISION="b9971133b2f7b7513904770d565b683a93fb3624"
fetch_dependency "$DIRECTORY" "$URL" "$REVISION"

printf '\033[0;31m'"\nALL OK"'\033[0m\n'
#d20747c7ad9d7898284123987babc310c07101d7 subprojects/mickstr/mickstr (V3.0.0-24-gd20747c)
#b9971133b2f7b7513904770d565b683a93fb3624 subprojects/raygui/raygui (4.0-170-gb997113)
