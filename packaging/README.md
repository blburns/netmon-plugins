# Packaging

Platform-specific packaging assets and helpers for NetMon Plugins.

Primary package generation uses **CPack** via CMake. Run from the project root:

```bash
make build
make package
# Output: dist/netmon-plugins-0.1.0.{deb,rpm,pkg,dmg,msi,zip}
```

## Directory layout

```
packaging/
├── linux/      # DEB and RPM post-install scripts
├── macos/      # macOS PKG/DMG helpers
└── windows/    # Windows MSI/WIX notes and helpers
```

## Supported formats

| Platform | Formats | Generator |
|----------|---------|-----------|
| Linux | DEB, RPM | CPack |
| macOS | PKG, DMG | CPack (productbuild, DragNDrop) |
| Windows | MSI, ZIP | CPack (WIX, ZIP) |

## Build options

Optional library support affects which plugins are included:

```bash
make package ENABLE_SSL=ON ENABLE_SNMP=ON ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_LDAP=ON
make package ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_MYSQL=OFF ENABLE_PGSQL=OFF ENABLE_LDAP=OFF
```

## Platform guides

- [Linux packaging](linux/README.md)
- [macOS packaging](macos/README.md)
- [Windows packaging](windows/README.md)

## Requirements

- **Linux**: `dpkg-deb`, `rpmbuild` (or distro equivalents)
- **macOS**: Xcode command-line tools
- **Windows**: WiX Toolset (for MSI), Visual Studio build tools
