#!/bin/bash

#!/usr/bin/env bash

set -e

# ============================================================================
# Generate compile_commands.json for IDE integration (clangd, Pylance, etc.)
# 
# This script generates the CMake compilation database for better IDE support.
# It works with the new CMake-based build system.
#
# Usage:
#   ./generate_compile_commands.sh          # Default (release mode)
#   ./generate_compile_commands.sh rebuild  # Force clean rebuild
# ============================================================================

# Configuration
BUILD_DIR="build"
FORCE_REBUILD="${1:-OFF}"

echo "▶ Generating compile_commands.json"
echo "▶ Build dir: ${BUILD_DIR}"
echo "▶ Force rebuild: ${FORCE_REBUILD}"

# Create build directory if it doesn't exist
if [ ! -d "${BUILD_DIR}" ]; then
    echo "▶ Creating build directory..."
    mkdir -p "${BUILD_DIR}"
fi

cd "${BUILD_DIR}"

# Clean build if requested
if [ "${FORCE_REBUILD}" = "rebuild" ]; then
    echo "▶ Cleaning previous build..."
    rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
fi

# Configure with CMake, ensuring compile commands are exported
echo "▶ Configuring CMake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON..."
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. 2>&1 | tail -5

# Optional: Build to ensure all compilation paths are captured
# Uncomment if you want a full build (takes longer)
# echo "▶ Building to ensure full command generation..."
# make -j$(nproc) 2>&1 | tail -10

cd ..

# Copy compile_commands.json to repository root for IDE integration
if [ -f "${BUILD_DIR}/compile_commands.json" ]; then
    cp "${BUILD_DIR}/compile_commands.json" ./compile_commands.json
    echo "✔ compile_commands.json successfully generated and copied to repository root"
    echo "✔ IDE tools (clangd, Pylance, etc.) should now have full C++ support"
    exit 0
else
    echo "✖ ERROR: compile_commands.json not found in ${BUILD_DIR}/"
    echo "✖ CMake configuration may have failed. Check the output above."
    exit 1
fi

