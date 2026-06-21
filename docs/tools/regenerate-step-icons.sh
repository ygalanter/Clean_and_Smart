#!/usr/bin/env bash
# Regenerate platform step icons from resources/images/icon_steps_100.png
# One nearest-neighbor downscale per size — no chained resizes.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MASTER="$ROOT/resources/images/icon_steps_100.png"
if [[ ! -f "$MASTER" ]]; then
  echo "Missing master: $MASTER" >&2
  exit 1
fi
for sz in 19 26 35; do
  convert "$MASTER" \
    -filter point -resize "${sz}x${sz}" \
    -fill white -opaque black \
    "PNG32:$ROOT/resources/images/icon_steps_${sz}.png"
  echo "icon_steps_${sz}.png"
done
