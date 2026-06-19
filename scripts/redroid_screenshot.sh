#!/usr/bin/bash
set -euo pipefail

ADB_HOST="localhost:5555"
OUTPUT="/tmp/r.jpg"

adb connect "$ADB_HOST" >/dev/null

if ! adb -s "$ADB_HOST" get-state >/dev/null 2>&1; then
    echo "error: device $ADB_HOST not available" >&2
    exit 1
fi

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

adb -s "$ADB_HOST" exec-out screencap -p > "$tmp"
mv "$tmp" "$OUTPUT"
trap - EXIT

echo "$OUTPUT"
