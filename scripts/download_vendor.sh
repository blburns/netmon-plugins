#!/bin/bash
# scripts/download_vendor.sh
# Script to manually download vendor protocol headers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
THIRD_PARTY_DIR="$PROJECT_ROOT/vendor"
INCLUDE_DIR="$THIRD_PARTY_DIR/include"

# Create directories
mkdir -p "$INCLUDE_DIR"

echo "Downloading vendor protocol headers..."

# Example: Download a protocol header
# Uncomment and modify as needed

# Example 1: Download a single header file
# echo "Downloading example_protocol.h..."
# curl -L -o "$INCLUDE_DIR/example_protocol.h" \
#     "https://raw.githubusercontent.com/example/protocol/master/include/protocol.h"

# Example 2: Download from a Git repository
# if [ ! -d "$THIRD_PARTY_DIR/src/example_protocol" ]; then
#     echo "Cloning example_protocol repository..."
#     git clone --depth 1 --branch v1.0.0 \
#         https://github.com/example/protocol.git \
#         "$THIRD_PARTY_DIR/src/example_protocol"
#     cp "$THIRD_PARTY_DIR/src/example_protocol/include/*.h" "$INCLUDE_DIR/"
# fi

# Example 3: Download and extract a tarball
# if [ ! -f "$INCLUDE_DIR/example_protocol.h" ]; then
#     echo "Downloading example_protocol tarball..."
#     curl -L -o /tmp/example_protocol.tar.gz \
#         "https://example.com/protocol-1.0.0.tar.gz"
#     tar -xzf /tmp/example_protocol.tar.gz -C /tmp/
#     cp /tmp/protocol-1.0.0/include/*.h "$INCLUDE_DIR/"
#     rm -rf /tmp/example_protocol.tar.gz /tmp/protocol-1.0.0
# fi

echo "Third-party headers downloaded to: $INCLUDE_DIR"
echo ""
echo "To use these headers in your plugins, include them like:"
echo "  #include \"vendor/example_protocol.h\""
echo ""
echo "Note: Make sure to add the include directory in CMakeLists.txt"

