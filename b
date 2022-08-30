#!/usr/bin/env bash
set -euo pipefail

PYTHON3=$(command -v python3 || true)
if [[ -x "$PYTHON3" ]]; then
    "$PYTHON3" build.py "$@"
else
    echo Python3 is required to build nanoprintf tests.
fi

