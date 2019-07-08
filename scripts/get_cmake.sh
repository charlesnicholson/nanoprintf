#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

CMAKE_DIR="$SCRIPT_PATH/../external/cmake"
HOST_OS=$(uname -s)
CMAKE_PREFIX=cmake-3.14.5-$HOST_OS-x86_64

if [ ! -f "$CMAKE_DIR/cmake" ]; then
    if [ "$HOST_OS" == "Darwin" ]; then
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/CMake.app/Contents/bin/cmake"
    else
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/bin/cmake"
    fi

    echo CMake not found at $CMAKE, retrieving...
    mkdir -p "$CMAKE_DIR"

    CMAKE_ARCHIVE="$CMAKE_PREFIX.tar.gz"
    rm -f "$CMAKE_DIR/$CMAKE_ARCHIVE"
    CMAKE_URL=https://cmake.org/files/v3.14/$CMAKE_ARCHIVE

    if [ "$HOST_OS" == "Darwin" ]; then
        curl -L -o "$CMAKE_DIR/$CMAKE_ARCHIVE" $CMAKE_URL
    else
        wget --no-check-certificate -P "$CMAKE_DIR" $CMAKE_URL
    fi

    # Verify SHA256 checksum because wget doesn't check server certificate
    (cd $CMAKE_DIR && sha256sum -c $SCRIPT_PATH/cmake.sha256sum < $CMAKE_ARCHIVE)
    if [ $? == 0 ]; then
    	tar xzf "$CMAKE_DIR/$CMAKE_ARCHIVE" -C "$CMAKE_DIR"
    	ln -s "$CMAKE" "$CMAKE_DIR/cmake"
    	rm "$CMAKE_DIR/$CMAKE_ARCHIVE"
    else
	echo "unknown sha256 checksum on $CMAKE_ARCHIVE"
	exit 1
    fi
fi

