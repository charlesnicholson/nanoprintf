#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

NINJA_DIR="$SCRIPT_PATH/../external/ninja"
NINJA="$NINJA_DIR/ninja"

if [ ! -f "$NINJA" ]; then
    echo Ninja not found at $NINJA, retrieving...
    mkdir -p "$NINJA_DIR"

    NINJA_URL_PREFIX=https://github.com/ninja-build/ninja/releases/download/v1.9.0

    HOST_OS=$(uname -s)
    if [ "$HOST_OS" == "Darwin" ]; then
        NINJA_ARCHIVE=ninja-mac.zip
        curl -L -o "$NINJA_DIR/ninja-mac.zip" "$NINJA_URL_PREFIX/$NINJA_ARCHIVE"
    else
        NINJA_ARCHIVE=ninja-linux.zip
        wget --no-check-certificate -P "$NINJA_DIR" "$NINJA_URL_PREFIX/$NINJA_ARCHIVE"
    fi

    unzip "$NINJA_DIR/$NINJA_ARCHIVE" -d "$NINJA_DIR"
    rm "$NINJA_DIR/$NINJA_ARCHIVE"
fi

