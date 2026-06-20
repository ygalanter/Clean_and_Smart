#!/usr/bin/env bash
# Phase 0 setup — run from repo root.
#
# What needs sudo: apt packages only (emulator libs, optional ImageMagick).
# The Pebble CLI uses a local Python 3.13 venv (no system python3.13 required).
# arm64 notes: docs/arm64-pebble-dev.md
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENV="$HOME/.local/pebble-sdk-venv"

echo "==> System packages (sudo) — emulator + ImageMagick"
sudo apt-get update
sudo apt-get install -y \
  libsdl2-2.0-0 libsndio7.0 \
  imagemagick

echo "==> uv (if missing)"
if ! command -v uv >/dev/null 2>&1; then
  curl -LsSf https://astral.sh/uv/install.sh | sh
fi
export PATH="$HOME/.local/bin:$PATH"

echo "==> Python 3.13 venv + pebble-tool"
# stpyv8 does not build on aarch64; use repo stub so pypkjs can launch for emulator install.
PY313="$(uv python find 3.13 2>/dev/null || true)"
if [[ -z "$PY313" ]]; then
  uv python install 3.13
  PY313="$(uv python find 3.13)"
fi
if [[ ! -d "$VENV" ]]; then
  "$PY313" -m venv "$VENV"
fi
source "$VENV/bin/activate"
pip install -U pip wheel
pip install /tmp/pebble_tool-5.0.38-py3-none-any.whl --no-deps 2>/dev/null || \
  pip download pebble-tool -d /tmp/pebble-dl --no-deps && \
  pip install /tmp/pebble-dl/pebble_tool-*.whl --no-deps
pip install \
  cobs colorama freetype-py google-auth google-auth-oauthlib httplib2 libpebble2 \
  oauth2client packaging pillow progressbar2 pyasn1 pyasn1-modules pypng pyqrcode \
  pyserial requests rsa six sourcemap websocket-client websockify wheel \
  gevent cryptography numpy netaddr sh progressbar2

echo "==> pypkjs (emulator install bridge; PKJS/Clay JS still unavailable on arm64)"
pip install "$REPO_ROOT/docs/tools/stpyv8_arm_stub" --no-deps
pip install pypkjs --no-deps
pip install peewee pygeoip python-dateutil backports.ssl-match-hostname gevent-websocket

ln -sf "$VENV/bin/pebble" "$HOME/.local/bin/pebble"
ln -sf "$VENV/bin/pebble" "$HOME/.local/bin/rebble"

echo "==> Pebble SDK"
pebble sdk install latest

echo "==> npm dependencies"
cd "$REPO_ROOT"
npm install

echo "==> Baseline build"
rebble build

echo ""
echo "Phase 0 complete."
echo "  Ensure PATH includes: \$HOME/.local/bin"
echo "  Optional emulator: rebble install --emulator basalt"
echo "  Note: arm64 uses stpyv8 stub — watchface C code runs; PKJS (settings/weather) does not."
