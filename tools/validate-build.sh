#!/bin/bash
# Verify all plugins in plugin_list.txt have built binaries

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
LIST_FILE="${ROOT_DIR}/plugin_list.txt"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: build directory not found: $BUILD_DIR" >&2
    echo "Run 'make build' first." >&2
    exit 1
fi

missing=0
total=0

while IFS= read -r plugin || [ -n "$plugin" ]; do
    plugin="${plugin//$'\r'/}"
    [ -z "$plugin" ] && continue
    total=$((total + 1))
    binary="${BUILD_DIR}/check_${plugin}"
    if [ ! -x "$binary" ] && [ ! -f "$binary" ] && [ ! -f "${binary}.exe" ]; then
        echo "MISSING: check_${plugin}" >&2
        missing=$((missing + 1))
    fi
done < "$LIST_FILE"

if [ "$missing" -gt 0 ]; then
    echo "" >&2
    echo "Validation failed: ${missing}/${total} plugins missing from ${BUILD_DIR}" >&2
    exit 1
fi

echo "Validation passed: ${total}/${total} plugins found in ${BUILD_DIR}"
