# Third-Party Headers and Libraries

This directory contains third-party headers and libraries that are downloaded automatically during the build process. These files are not included in the repository to keep it clean and avoid licensing issues.

## Structure

```
third_party/
├── include/          # Downloaded header files
├── lib/             # Downloaded libraries (if needed)
├── src/             # Downloaded source files (if needed)
└── CMakeLists.txt   # CMake configuration for downloading
```

## How It Works

Third-party dependencies are downloaded automatically using CMake's `FetchContent` module during the build process. The files are cached in the build directory and reused across builds.

## Adding New Third-Party Dependencies

To add a new third-party dependency:

1. Add the download configuration to `third_party/CMakeLists.txt`
2. Include the headers in your plugin using `#include "third_party/<name>/<header.h>"`
3. Link against the library if needed in the main `CMakeLists.txt`

## Example

See `third_party/CMakeLists.txt` for examples of how to download headers from GitHub, GitLab, or other sources.

