#!/usr/bin/env bash
# rebble kill only stops processes in pb-emulator.json; interrupted runs leave
# orphaned emery QEMU/pypkjs. Stale SPI flash or orphans show as a stuck pebbleOS
# boot bar (~10%) in the QEMU window.
set -euo pipefail

rebble kill --force 2>/dev/null || true
pkill -9 -f 'qemu-pebble.*pebble/emery' 2>/dev/null || true
pkill -9 -f 'pypkjs.*pebble-sdk/.*/emery' 2>/dev/null || true
sleep 1

# ~/.local/share/pebble-sdk/<sdk-version>/emery — no pebble_tool import needed
shopt -s nullglob
for persist in "${HOME}/.local/share/pebble-sdk"/*/emery; do
  rm -rf "$persist"
done
