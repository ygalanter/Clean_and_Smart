# Pebble development on arm64 (WSL2)

Notes for building and smoke-testing Clean & Smart on **Linux aarch64** (e.g. WSL2 on Apple Silicon or ARM Windows). Verified on WSL2 Ubuntu arm64 with Pebble SDK **4.9.169** and `pebble-tool` **5.0.38**.

Use this doc for **initial setup** on a fresh machine and as a reference for arm64-specific limitations.

---

## Quick reference

| Task | Works on arm64? |
|---|---|
| `rebble build` → `.pbw` | Yes |
| Emulator install (`rebble install --emulator basalt`) | Yes (with stub; see below) |
| Watchface **C code** in emulator | Yes |
| **PKJS** in emulator (Clay settings, weather API) | No (local WSL); yes in [CloudPebble](#optional-cloudpebble-remix) |
| `rebble install --phone <ip>` on real hardware | Yes (PKJS runs on the phone) |
| **CloudPebble** (browser IDE + emulator + PKJS) | Yes — no local stpyv8 needed |
| ImageMagick icon resize | Yes (after `apt install imagemagick`) |

---

## Fresh machine bootstrap

Run from the repo root. Adjust paths if your clone lives elsewhere.

```bash
# System packages (sudo)
sudo apt-get update
sudo apt-get install -y libsdl2-2.0-0 libsndio7.0 imagemagick

# uv + Python 3.13 venv
curl -LsSf https://astral.sh/uv/install.sh | sh   # skip if uv already installed
export PATH="$HOME/.local/bin:$PATH"
uv python install 3.13
uv python find 3.13 | xargs -I{} {} -m venv ~/.local/pebble-sdk-venv
source ~/.local/pebble-sdk-venv/bin/activate
pip install -U pip wheel

# pebble-tool (install deps manually; omit pypkjs — stpyv8 does not build on arm64)
pip download pebble-tool -d /tmp/pebble-dl --no-deps
pip install /tmp/pebble-dl/pebble_tool-*.whl --no-deps
pip install \
  cobs colorama freetype-py google-auth google-auth-oauthlib httplib2 libpebble2 \
  oauth2client packaging pillow progressbar2 pyasn1 pyasn1-modules pypng pyqrcode \
  pyserial requests rsa six sourcemap websocket-client websockify wheel \
  gevent cryptography numpy netaddr sh

# pypkjs bridge (stub stpyv8 — see below)
pip install docs/tools/stpyv8_arm_stub --no-deps
pip install pypkjs --no-deps
pip install peewee pygeoip python-dateutil backports.ssl-match-hostname gevent-websocket

ln -sf ~/.local/pebble-sdk-venv/bin/pebble ~/.local/bin/pebble
ln -sf ~/.local/pebble-sdk-venv/bin/pebble ~/.local/bin/rebble

pebble sdk install latest
npm install
rebble build
```

If anything was created as root, fix ownership (see [File ownership](#file-ownership) below).

---

## Environment choices

### Python 3.13 via `uv`

System Python may be too new (e.g. 3.14) for a full `pebble-tool` install. Use a dedicated venv:

- **Venv:** `~/.local/pebble-sdk-venv`
- **CLI symlinks:** `~/.local/bin/pebble` and `~/.local/bin/rebble` (Makefile uses `rebble`)

Ensure `~/.local/bin` is on your `PATH` (usually via `~/.bashrc`).

### System packages (sudo)

Required for the SDL emulator window and Phase 1 asset work:

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-2.0-0 libsndio7.0 imagemagick
```

Builds compile without these; the emulator needs SDL.

### File ownership

If setup was ever run as root, fix ownership before using `pebble`/`rebble`:

```bash
sudo chown -R "$USER:$USER" \
  ~/misc/pt2-watchfaces/Clean_and_Smart \
  ~/.local/pebble-sdk-venv \
  ~/.local/share/pebble-sdk
```

Symptoms of wrong ownership: `Permission denied` on `~/.local/share/pebble-sdk/settings.json` or `pending_analytics.json`.

---

## The pypkjs / stpyv8 problem

`rebble install --emulator` needs **pypkjs** — a Python process that bridges the Pebble tool to the QEMU emulator. pypkjs depends on **stpyv8** (Google V8 bindings), which **does not build on aarch64**.

### Symptom

```
Couldn't launch pypkjs:
.../python3.13: No module named pypkjs
```

Or, if pypkjs is missing entirely: the emulator window opens but shows **“install an app to continue”** and install exits with an error.

### Workaround (used in this repo)

Install a **stub** package that satisfies the `stpyv8` import without providing a JS engine, then install `pypkjs` without pulling the real stpyv8 build:

```bash
source ~/.local/pebble-sdk-venv/bin/activate
pip install docs/tools/stpyv8_arm_stub --no-deps
pip install pypkjs --no-deps
pip install peewee pygeoip python-dateutil backports.ssl-match-hostname gevent-websocket
```

The stub lives at [`docs/tools/stpyv8_arm_stub/`](tools/stpyv8_arm_stub/). The bootstrap block above installs it; or run the pip lines manually after activating the venv.

**What the stub enables:** pypkjs starts, connects to QEMU, and **installs the `.pbw`**. Native watchface code runs in the emulator.

**What the stub does not enable:** executing PKJS (JavaScript). When the watch asks for phone-side JS, V8 is unavailable. Expect:

- No Clay settings page in the emulator
- No weather fetch via PKJS in the emulator
- Possible log noise if the app expects JS ready handshake (usually non-fatal for display smoke tests)

This is sufficient for verifying layout, fonts, and C-side logic before feature work.

---

## Emulator smoke test

From the repo root:

```bash
export PATH="$HOME/.local/bin:$PATH"
rebble build
rebble kill --force          # clear stale QEMU/pypkjs from a failed attempt
rebble install --emulator basalt
```

You should see **Clean & Smart** in the SDL window: day name, time, date. Weather may be absent — that is expected on arm64.

Other platforms: `aplite`, `chalk`, `diorite`, `emery` (see `Makefile` targets).

### Stale emulator processes

If install behaves oddly, kill everything and retry:

```bash
rebble kill --force
# or manually: pkill -f qemu-pebble
```

### Emery install (Pebble Time 2)

Emery’s larger resource pack can overrun the QEMU serial buffer. Without throttling, install often fails silently at `-v` (`App install failed.`). A heavy throttle (e.g. `0.005`) works but can take ~2 minutes.

**Recommended** — fast and reliable (~3–30 s):

```bash
rebble kill --force
rebble build
rebble install --emulator emery --throttle 0.001
```

Or `make emery` (includes clean/wipe/build). Use `-vvv` only when debugging; single `-v` hides the real error.

---

## Testing PKJS and settings

PKJS **does** run on a real phone when you install over the developer connection:

```bash
rebble install --phone <phone-ip>
```

Use this for Clay settings and weather during Phase 2+ if emulator PKJS is required.

**Visual-only Clay preview (arm64):** PKJS does not run in the WSL emulator, but you can preview the settings **layout** in Chrome on Windows — see [Clay settings visual check](#clay-settings-visual-check) below.

**Full PKJS in emulator (arm64):** use [CloudPebble](#optional-cloudpebble-remix) — same repo, browser build/emulator, no local stpyv8.

**Alternative:** x86_64 Linux VM or Docker with a full Pebble SDK install (real stpyv8). Community images such as [ClusterM/pebble-dev-arm-linux](https://github.com/ClusterM/pebble-dev-arm-linux) document similar limitations on native arm64 and use containers for amd64.

---

## Optional: CloudPebble remix

The [Rebble app store listing](https://apps.rebble.io) for Clean & Smart shows **“Remix on CloudPebble!”** — that is the original author’s one-click link to open the GitHub source in the browser IDE. Same project as this repo; CloudPebble is optional, not required.

[CloudPebble](https://developer.repebble.com/sdk/cloud) is a browser-based Pebble IDE: edit, compile, emulate, and install without local `pebble-tool` or stpyv8. **PKJS and Clay settings work in the cloud emulator**, which local arm64 WSL does not provide.

### When to use it

| Use CloudPebble | Stay local (this doc) |
|---|---|
| Phase 2: full Clay settings + save flow in emulator | Primary C refactor, git, Cursor, Makefile/`rebble` |
| Weather PKJS smoke test in emulator | Day-to-day builds and watchface layout |
| No local SDK setup | Docs, scripts, upstream PR workflow |

You can use **both**: develop C locally, push to GitHub, pull/build in CloudPebble for settings testing.

### Import this fork

1. Sign in at [cloudpebble.repebble.com](https://cloudpebble.repebble.com).
2. Open the import URL for this fork:  
   [cloudpebble.repebble.com/ide/import/github/yaronf/Clean_and_Smart](https://cloudpebble.repebble.com/ide/import/github/yaronf/Clean_and_Smart)
3. Build → run the **basalt** emulator → open watchface **Settings** to test Clay.

**Import error `Invalid semver 2.50`:** CloudPebble requires strict semver (`major.minor.patch`). This repo uses `2.50.0` in `package.json` (legacy Pebble faces often used `2.50`, which local `rebble build` accepts but CloudPebble rejects).

Upstream (original author):  
[cloudpebble.repebble.com/ide/import/github/ygalanter/Clean_and_Smart](https://cloudpebble.repebble.com/ide/import/github/ygalanter/Clean_and_Smart)

### Sync with local work

After local commits, use CloudPebble’s **GitHub pull** (or re-import) before testing. After editing in CloudPebble, **push to GitHub** and `git pull` locally so the fork stays one source of truth.

---

## Clay settings visual check

Use this during **Phase 2** when editing [`src/pkjs/config.json`](../src/pkjs/config.json). It checks labels, sections, and control types — **not** save-to-watch or persistence (that still needs a phone; see above).

### 1. Build and generate preview HTML

From the repo root in WSL:

```bash
rebble build
node docs/tools/clay-preview-url.js
# prints: .../build/clay-preview.html
```

### 2. Open in Chrome (Windows)

The HTML file lives under WSL. Open it from Windows Chrome, for example:

- **File → Open** and paste the WSL path, e.g.  
  `\\wsl$\Ubuntu\home\yaronf\misc\pt2-watchfaces\Clean_and_Smart\build\clay-preview.html`  
  (adjust distro/username if yours differ), or
- Copy to a Windows folder:  
  `cp build/clay-preview.html /mnt/c/Users/<You>/Downloads/clay-preview.html`  
  then open from Downloads in Chrome.

Use a **phone-sized** narrow window (~200px wide) to approximate the Pebble settings WebView.

### 3. What to expect

- You should see the Clay page (dark theme, orange accents) with all sections from `config.json`.
- **Submit / Save** may not return to a watch — there is no PKJS bridge in this preview. That is fine for layout review.
- After changing `config.json`, re-run `node docs/tools/clay-preview-url.js` and refresh the browser.

### Limitations

| Works in preview | Does not |
|---|---|
| Section headings, labels, selects, toggles, colors | Settings applied to emulator watch |
| Default values from `config.json` | `localStorage` / saved phone settings |
| Layout after editing `config.json` | Weather fetch, geolocation, full PKJS flow |

---

## Related files

| Path | Purpose |
|---|---|
| [`docs/tools/stpyv8_arm_stub/`](tools/stpyv8_arm_stub/) | stpyv8 stub for arm64 |
| [`docs/tools/clay-preview-url.js`](tools/clay-preview-url.js) | Generate Clay preview HTML for Chrome |
| [`configurable-display-rows.md`](configurable-display-rows.md) | Feature plan (Phase 0 checklist links here) |
