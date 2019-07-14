#!/usr/bin/env bash
set -euo pipefail
set -x

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

# Find or download cmake, ensure it exists.
CMAKE=$(which cmake)
if [ ! -x "$CMAKE" ]; then
    (exec scripts/get_cmake.sh)
    CMAKE="$SCRIPT_PATH/external/cmake/cmake"
fi

# Find or download ninja, ensure it exists.
NINJA=$(which ninja)
if [ ! -x "$NINJA" ]; then
    (exec scripts/get_ninja.sh)
    NINJA="$SCRIPT_PATH/external/ninja/ninja"
fi

# Default to release builds, override via first arg to this script.
BUILD_TYPE=Release
if [ $# -gt 0 ] && [ -n "$1" ]; then
    BUILD_TYPE=$1; shift
fi

# Ensure the output directory exists for CMake to configure / build into.
BUILD_PATH="$SCRIPT_PATH/build/ninja/$BUILD_TYPE"
#[ ! -d "$BUILD_PATH" ] && mkdir -p "$BUILD_PATH"

# Configure CMake
[ ! -d "$BUILD_PATH"/CMakeFiles ] &&
    "$CMAKE" \
        -S "$SCRIPT_PATH" \
        -B "$BUILD_PATH" \
        -G Ninja \
        -DCMAKE_MAKE_PROGRAM="$NINJA" \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build nanoprintf.
"$CMAKE" --build "$BUILD_PATH" -- "$@"

