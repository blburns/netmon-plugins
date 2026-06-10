#!/bin/sh
# Post-install script for NetMon Plugins (DEB/RPM)
set -e

PLUGIN_DIR="${PLUGIN_DIR:-/usr/local/libexec/monitoring-plugins}"

if [ -d "$PLUGIN_DIR" ]; then
    chmod 755 "$PLUGIN_DIR"/check_* 2>/dev/null || true
    echo "NetMon Plugins: set permissions on plugins in $PLUGIN_DIR"
fi

CONFIG_DIR="${CONFIG_DIR:-/usr/local/etc/netmon-plugins}"
if [ ! -d "$CONFIG_DIR" ] && [ -d "$(dirname "$0")/../../config" ]; then
    mkdir -p "$CONFIG_DIR"
    for f in netmon-plugins.conf.example plugins.defaults.conf.example; do
        src="$(dirname "$0")/../../config/$f"
        if [ -f "$src" ] && [ ! -f "$CONFIG_DIR/${f%.example}" ]; then
            cp "$src" "$CONFIG_DIR/${f%.example}"
            echo "NetMon Plugins: installed default $CONFIG_DIR/${f%.example}"
        fi
    done
fi

echo "NetMon Plugins installed successfully."
