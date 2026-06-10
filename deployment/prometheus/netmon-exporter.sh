#!/bin/bash
# NetMon Plugins - Prometheus textfile collector exporter
# Install: sudo cp netmon-exporter.sh /usr/local/bin/ && sudo chmod 755 /usr/local/bin/netmon-exporter.sh
# Cron:    */1 * * * * /usr/local/bin/netmon-exporter.sh

set -euo pipefail

PLUGIN_DIR="${PLUGIN_DIR:-/usr/local/libexec/monitoring-plugins}"
METRICS_DIR="${METRICS_DIR:-/var/lib/prometheus/node-exporter}"
METRICS_FILE="${METRICS_DIR}/netmon.prom"

mkdir -p "$METRICS_DIR"

write_metric() {
    local name="$1"
    local labels="$2"
    local value="$3"
    echo "${name}{${labels}} ${value}" >> "$METRICS_FILE"
}

TMP_FILE="$(mktemp)"
trap 'rm -f "$TMP_FILE"' EXIT

{
    echo "# HELP netmon_check_status Plugin exit code (0=OK, 1=WARNING, 2=CRITICAL, 3=UNKNOWN)"
    echo "# TYPE netmon_check_status gauge"
} > "$TMP_FILE"

if [ -x "${PLUGIN_DIR}/check_disk" ]; then
    "${PLUGIN_DIR}/check_disk" -w 80 -c 90 / >/dev/null 2>&1 || true
    write_metric "netmon_check_status" 'check="disk",path="/"' "$?"
fi

if [ -x "${PLUGIN_DIR}/check_load" ]; then
    "${PLUGIN_DIR}/check_load" -w 1.0,2.0,3.0 -c 2.0,4.0,6.0 >/dev/null 2>&1 || true
    write_metric "netmon_check_status" 'check="load"' "$?"
fi

if [ -x "${PLUGIN_DIR}/check_uptime" ]; then
    "${PLUGIN_DIR}/check_uptime" >/dev/null 2>&1 || true
    write_metric "netmon_check_status" 'check="uptime"' "$?"
fi

mv "$TMP_FILE" "$METRICS_FILE"
