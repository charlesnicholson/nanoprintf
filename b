#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

(exec scripts/get_cmake.sh)
(exec scripts/get_ninja.sh)

CMAKE="$SCRIPT_PATH/external/cmake/cmake"
NINJA="$SCRIPT_PATH/external/ninja/ninja"

BUILD_TYPE=relwithdebinfo
if [ -n "$1" ]; then
    BUILD_TYPE=$1; shift
fi

BUILD_PATH="$SCRIPT_PATH/build/ninja/$BUILD_TYPE"
[ ! -d "$BUILD_PATH" ] && mkdir -p "$BUILD_PATH"
[ ! -d "$BUILD_PATH"/CMakeFiles ] &&
    (cd "$BUILD_PATH";
     "$CMAKE" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G Ninja -DCMAKE_MAKE_PROGRAM="$NINJA" "$SCRIPT_PATH")
(cd "$BUILD_PATH"; "$NINJA" $@)

