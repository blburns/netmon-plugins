# macOS Packaging

NetMon Plugins builds PKG and DMG packages via CPack on macOS.

## Quick build

```bash
make build
make package
ls dist/*.pkg dist/*.dmg
```

## Install from package

```bash
sudo installer -pkg dist/netmon-plugins-0.1.0.pkg -target /
```

Plugins install to `/usr/local/libexec/monitoring-plugins/`.

## Dependencies

```bash
brew install cmake openssl
```

## Code signing (optional)

For distribution outside your organization, sign packages with your Apple Developer ID:

```bash
productsign --sign "Developer ID Installer: Your Name" \
    dist/netmon-plugins-0.1.0-unsigned.pkg \
    dist/netmon-plugins-0.1.0-signed.pkg
```

## Notes

- CPack `productbuild` and `DragNDrop` generators are configured in `CMakeLists.txt`.
- Use `make install` for local development without packaging.
