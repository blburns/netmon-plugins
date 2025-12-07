# Vendor Headers and Libraries

This directory contains vendor (third-party) headers and libraries that are downloaded automatically during the build process. These files are not included in the repository to keep it clean and avoid licensing issues.

Each vendor library (e.g., MySQL, PostgreSQL, Oracle) can be extracted/containerized in its own subdirectory for better organization.

## Structure

```
vendors/
├── include/          # Downloaded header files
├── lib/              # Downloaded libraries (if needed)
├── src/              # Downloaded source files (if needed)
├── mysql/            # MySQL client library (containerized)
├── postgresql/       # PostgreSQL (libpq) client library (containerized)
├── oracle/           # Oracle Instant Client (containerized)
└── CMakeLists.txt    # CMake configuration for downloading
```

## How It Works

Vendor dependencies are downloaded automatically using CMake's `FetchContent` module during the build process. The files are cached in the build directory and reused across builds.

Each vendor library can be organized in its own subdirectory (e.g., `vendors/mysql/`, `vendors/postgresql/`) for containerized/extracted libraries, making it easier to manage multiple vendor dependencies.

## Adding New Vendor Dependencies

To add a new vendor dependency:

1. Add the download configuration to `vendors/CMakeLists.txt`
2. Optionally create a subdirectory for the vendor library (e.g., `vendors/mysql/`)
3. Include the headers in your plugin using `#include "vendors/<name>/<header.h>"` or `#include "vendors/<name>.h"`
4. Link against the library if needed in the main `CMakeLists.txt`

## Example Vendor Libraries

- **MySQL** - MySQL client library headers
- **PostgreSQL** - PostgreSQL (libpq) client library headers
- **Oracle** - Oracle Instant Client headers
- **Protocol Headers** - Various protocol-specific headers

See `vendors/CMakeLists.txt` for examples of how to download headers from:
- GitHub/GitLab repositories
- Direct URLs (single header files)
- Tarballs/archives

## Manual Download

You can also manually download vendor headers using the provided scripts:
- `scripts/download_vendor.sh` (Linux/macOS)
- `scripts/download_vendor.ps1` (Windows PowerShell)
