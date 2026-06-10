#!/bin/bash
# Run a NetMon plugin check with optional logging

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <plugin> [plugin-args...]" >&2
    echo "Example: $0 disk -w 80 -c 90 /" >&2
    exit 1
fi

PLUGIN_NAME="$1"
shift

if [ -n "${PLUGIN_DIR:-}" ] && { [ -x "${PLUGIN_DIR}/check_${PLUGIN_NAME}" ] || [ -f "${PLUGIN_DIR}/check_${PLUGIN_NAME}" ]; }; then
    :
elif [ -x "${ROOT_DIR}/build/check_${PLUGIN_NAME}" ] || [ -f "${ROOT_DIR}/build/check_${PLUGIN_NAME}" ]; then
    PLUGIN_DIR="${ROOT_DIR}/build"
elif [ -x "/usr/local/libexec/monitoring-plugins/check_${PLUGIN_NAME}" ]; then
    PLUGIN_DIR="/usr/local/libexec/monitoring-plugins"
else
    PLUGIN_DIR="${PLUGIN_DIR:-${ROOT_DIR}/build}"
fi

PLUGIN_BIN="${PLUGIN_DIR}/check_${PLUGIN_NAME}"

if [ ! -x "$PLUGIN_BIN" ] && [ ! -f "$PLUGIN_BIN" ]; then
    echo "Error: plugin not found: $PLUGIN_BIN" >&2
    exit 3
fi

if [ -n "${LOG_FILE:-}" ]; then
    {
        echo "=== $(date -Iseconds) check_${PLUGIN_NAME} $* ==="
        "$PLUGIN_BIN" "$@" 2>&1
        echo "exit_code=$?"
    } | tee -a "$LOG_FILE"
    exit "${PIPESTATUS[0]:-0}"
fi

exec "$PLUGIN_BIN" "$@"
