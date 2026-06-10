#!/bin/bash
# List all NetMon plugins defined in plugin_list.txt

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LIST_FILE="${ROOT_DIR}/plugin_list.txt"

if [ ! -f "$LIST_FILE" ]; then
    echo "Error: plugin_list.txt not found at $LIST_FILE" >&2
    exit 1
fi

count=0
while IFS= read -r plugin || [ -n "$plugin" ]; do
    plugin="${plugin//$'\r'/}"
    [ -z "$plugin" ] && continue
    printf "check_%s\n" "$plugin"
    count=$((count + 1))
done < "$LIST_FILE"

echo "" >&2
echo "Total: ${count} plugins" >&2
