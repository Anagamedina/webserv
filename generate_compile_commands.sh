#!/bin/bash

#!/usr/bin/env bash

set -e

# ---------------- configuration ----------------
BUILD_DIR="build"
USE_MANUAL="${1:-OFF}"   # pass ON as first arg to enable manual mode
# -----------------------------------------------

echo "▶ Generating compile_commands.json"
echo "▶ Build dir: ${BUILD_DIR}"
echo "▶ Manual mode: ${USE_MANUAL}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [ "${USE_MANUAL}" = "ON" ]; then
    cmake -DUSE_MANUAL=ON ..
else
    cmake ..
fi

# Build is not strictly required, but ensures full command generation
#cmake --build .

cd ..

if [ -f "${BUILD_DIR}/compile_commands.json" ]; then
    cp "${BUILD_DIR}/compile_commands.json" ./compile_commands.json
    echo "✔ compile_commands.json copied to repository root"
else
    echo "✖ compile_commands.json not found!"
    exit 1
fi

