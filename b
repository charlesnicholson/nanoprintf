#!/usr/bin/env bash
set -euo pipefail

if command -v python3 &>/dev/null; then
    python3 build.py "$@"
else
    echo Python3 is required to build nanoprintf tests.
fi

