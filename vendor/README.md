# Vendor Headers and Libraries

This directory contains vendor (third-party) headers and libraries that are downloaded automatically during the build process. These files are not included in the repository to keep it clean and avoid licensing issues.

## Structure

```
vendor/
├── include/          # Downloaded header files
├── lib/             # Downloaded libraries (if needed)
├── src/             # Downloaded source files (if needed)
└── CMakeLists.txt   # CMake configuration for downloading
```

## How It Works

Vendor dependencies are downloaded automatically using CMake's `FetchContent` module during the build process. The files are cached in the build directory and reused across builds.

## Adding New Vendor Dependencies

To add a new vendor dependency:

1. Add the download configuration to `vendor/CMakeLists.txt`
2. Include the headers in your plugin using `#include "vendor/<name>.h"` or `#include "vendor/<name>/<header.h>"`
3. Link against the library if needed in the main `CMakeLists.txt`

## Examples

See `vendor/CMakeLists.txt` for examples of how to download headers from:
- GitHub/GitLab repositories
- Direct URLs (single header files)
- Tarballs/archives

## Manual Download

You can also manually download vendor headers using the provided scripts:
- `scripts/download_vendor.sh` (Linux/macOS)
- `scripts/download_vendor.ps1` (Windows PowerShell)

