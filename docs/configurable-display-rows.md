# Configurable Top/Bottom Display Rows

Design notes and implementation record for the Clean & Smart row refactor (shipped in v2.51.0).

**Status:** All phases complete (Phase 4 testing: 12/12). Post-ship: `(empty)` row option added (mode 4).

**Build system:** `rebble` is a symlink to `pebble` in `~/.local/bin` (Makefile unchanged).

---

## Implementation phases (order of work)

| Phase | What | Done |
|---|---|---|
| **0** | [Dev environment & tooling](#phase-0-dev-environment--tooling) | yes |
| **1** | [Assets](#phase-1-assets) — step icon PNG | yes |
| **2** | [Settings plumbing](#settings--persistence) — message keys, Clay, PKJS | yes |
| **3** | [Core C refactor](#architecture) — rows, formatters, health, focus | yes |
| **4** | [Testing](#testing-checklist) | yes |

---

## Phase 0: Dev environment & tooling

Completed before feature work. Goal was a clean build of the pre-refactor watchface on the dev machine.

Setup guide: [`arm64-pebble-dev.md`](arm64-pebble-dev.md) (bootstrap, pypkjs/stpyv8 workarounds, emulator smoke test).

### Environment

- **OS:** WSL2 Ubuntu (arm64)
- **Node.js:** v22+ and npm — installed
- **Python:** 3.13 via `uv` in `~/.local/pebble-sdk-venv` (system is 3.14, too new for full `pebble-tool` install)

### System packages (sudo — you run this)

Needed for **emulator** and icon resize:

```bash
sudo apt-get update
sudo apt-get install -y libsdl2-2.0-0 libsndio7.0 imagemagick
```

Build compiles without these; emulator needs SDL.

### Pebble CLI + SDK (no sudo)

See [`arm64-pebble-dev.md`](arm64-pebble-dev.md) for the full pypkjs/stpyv8 story. Summary: emulator install works; PKJS (settings/weather) does not run in the emulator on arm64.

```bash
export PATH="$HOME/.local/bin:$PATH"
source ~/.local/pebble-sdk-venv/bin/activate   # optional; rebble/pebble also symlinked in ~/.local/bin
pebble --version
pebble sdk list
```

Makefile compatibility:

```bash
rebble build    # symlink to pebble
```

SDK installed: **4.9.169** at `~/.local/share/pebble-sdk/`.

### Project npm dependencies

```bash
npm install
```

### Baseline build (verified)

```bash
rebble build
# Output: build/Clean_and_Smart.pbw
```

### Emulator smoke test

See [`arm64-pebble-dev.md`](arm64-pebble-dev.md#emulator-smoke-test).

```bash
rebble kill --force
rebble install --emulator basalt
```

### Phase 0 checklist

- [x] `uv` + Python 3.13 venv + `pebble-tool`
- [x] `pebble sdk install latest`
- [x] `rebble` symlink for Makefile
- [x] `npm install`
- [x] `rebble build` succeeds
- [x] `sudo apt` emulator libs + ImageMagick
- [x] Emulator install (`rebble install --emulator basalt`)

---

## Phase 1: Assets

**Done.** Platform-specific step icons registered as `ICON_STEPS` in `package.json` and wired in C code.

| Platform | File | Size |
|---|---|---|
| `aplite`, `basalt`, `diorite`, `emery` | `icon_steps_26.png` | 26×26 |
| `chalk` | `icon_steps_19.png` | 19×19 |

`aplite` is round but uses the 26 px asset; only `chalk` uses the 19 px variant.

Source/reference art: `icon_steps_100.png`.

### Step icon (reference)

Created from the approved two-shoe-prints design. To regenerate rect icon from source:

```bash
convert resources/images/icon_steps_100.png -resize 26x26 -background none -gravity center -extent 26x26 \
  -fuzz 10% -transparent white -colorspace Gray -threshold 50% \
  PNG32:resources/images/icon_steps_26.png
```

Registered in [`package.json`](../package.json) (rect example):

```json
{
  "file": "images/icon_steps_26.png",
  "name": "ICON_STEPS",
  "targetPlatforms": ["aplite", "basalt", "diorite", "emery"],
  "type": "bitmap"
}
```

---

## Goal

Replaced the rigid layout in [`src/c/main.c`](../src/c/main.c):

```mermaid
flowchart TB
  subgraph before [Before]
    dow["text_dow: full day name"]
    time["text_time: HH:MM permanent"]
    date["text_date: full date"]
    dow --> time --> date
  end

  subgraph after [After — shipped]
    top["text_row_top: configurable"]
    time2["text_time: HH:MM permanent"]
    bot["text_row_bottom: configurable"]
    top --> time2 --> bot
  end
```

Each row independently selects one of five modes:

| Value | Mode | Example output |
|---|---|---|
| 0 | Full day name | `WEDNESDAY` / `onsdag` |
| 1 | Full date | `DEC-10-2015` (uses existing `KEY_DATE_FORMAT`) |
| 2 | Step count | `[foot] 8432` — step icon + number; `--` or `!!!` when unavailable/overflow |
| 3 | Abbreviated DOW + abbreviated date | `SAT 20-JUN-2026` — **both** parts abbreviated, combined in one row |
| 4 | Empty | Row hidden (no text, no icon) |

**Defaults (new installs):** top = 0 (full DOW), bottom = 1 (full date) — matches pre-refactor behavior.

---

## Architecture

### Row rendering pipeline

Implemented in [`src/c/main.c`](../src/c/main.c):

```c
typedef enum {
  ROW_FULL_DOW = 0,
  ROW_FULL_DATE = 1,
  ROW_STEPS = 2,
  ROW_ABBR_DOW_DATE = 3,
  ROW_EMPTY = 4,
} RowDisplayMode;

static void format_full_dow(char *buf, size_t len, struct tm *t);
static void format_full_date(char *buf, size_t len, struct tm *t);
static void get_abbr_dow(char *buf, size_t len, struct tm *t);
static void get_abbr_month(char *buf, size_t len, struct tm *t);
static void format_abbr_dow_date(char *buf, size_t len, struct tm *t);
static void format_steps(char *buf, size_t len);
static void layout_empty_row(TextLayer *text, BitmapLayer *icon);
static void layout_step_row(TextLayer *text, BitmapLayer *icon, GRect full_frame, const char *text_str);
static void layout_text_row(TextLayer *text, BitmapLayer *icon, GRect full_frame,
                            GTextAlignment align, const char *text_str);
static void update_row(TextLayer *text, BitmapLayer *icon, GRect full_frame,
                       GTextAlignment text_align, RowDisplayMode mode, struct tm *tick_time,
                       char *buf, size_t buf_len);
static void refresh_steps_rows(void);
static void refresh_rows(struct tm *tick_time, TimeUnits units_changed);
```

- **`format_full_date`** — existing `switch (flag_dateFormat)` logic; behavior unchanged from pre-refactor.
- **`format_full_dow`** — custom `LANG_DAY[]` when `flag_language != LANG_DEFAULT`, else `strftime(..., "%A", ...)`.
- **`format_abbr_dow_date`** — **abbreviated DOW + abbreviated date** in one string (mode 3):
  - **Abbrev DOW:** `%a` / `LANG_DAY_ABBR[]` (≤3 chars).
  - **Abbrev date:** 3-char month from `%b` / `LANG_MONTH_UPPER[]`; day + year per `KEY_DATE_FORMAT`. Never `%B` or `%A`.
  - **Combined layout** per `KEY_DATE_FORMAT`:
    - Format 0 (MDY): `{ABBR_DOW} {MON}-{DD}-{YYYY}` e.g. `SAT JUN-20-2026`
    - Format 1 (DMY): `{ABBR_DOW} {DD}-{MON}-{YYYY}` e.g. `SAT 20-JUN-2026`
    - Format 2 (YMD): `{ABBR_DOW} {YYYY}-{MM}-{DD}` e.g. `SAT 2026-06-20`
- **`format_steps`** — checks `health_service_metric_accessible` first; shows `"--"` when unavailable; overflow → `"!!!"`:

```c
HealthServiceAccessibilityMask mask = health_service_metric_accessible(
    HealthMetricStepCount, now - SECONDS_PER_DAY, now);
if (!(mask & HealthServiceAccessibilityMaskAvailable)) {
  strncpy(buf, "--", ...);
  return;
}
HealthValue steps = health_service_sum_today(HealthMetricStepCount);
if (steps > 99999) {
  strncpy(buf, "!!!", ...);
} else {
  snprintf(buf, ..., "%u", (unsigned int)steps);
}
```

- **`layout_empty_row`** — hides text layer and step icon (mode 4).
- **`layout_step_row`** / **`layout_text_row`** — unhide text layer when switching back from empty.

### Step icon (mode 2 only)

When a row is in step mode, show footsteps icon **left** of the count (including `--` and `!!!`).

```mermaid
flowchart LR
  subgraph stepRow [Step row layout]
    icon["icon_steps"]
    gap["8px gap"]
    text["8432 / -- / !!!"]
    icon --> gap --> text
  end
```

- Platform-specific assets in [`package.json`](../package.json): `icon_steps_26.png` (rect platforms), `icon_steps_19.png` (`chalk`). Source art: `icon_steps_100.png`.
- Load `RESOURCE_ID_ICON_STEPS`, tint via `tint_step_icon()` (mirrors `tint_meteoicon()`).
- `step_icon_top` / `step_icon_bottom` `BitmapLayer`s — hidden when row is not in step mode.
- `layout_step_row()` centers icon+text group in row bounds; `STEP_ICON_GAP` is 8 px.

| Old | New | Rect (`PBL_RECT`) | Round (`chalk` / `aplite`) |
|---|---|---|---|
| `text_dow` | `text_row_top` + `step_icon_top` | y ≈ 30, full width, centered | y ≈ 23, full width, centered |
| `text_date` | `text_row_bottom` + `step_icon_bottom` | y ≈ 129, full width, centered | GRect(35, 111, 80, 27), left-aligned |

### Update triggers

| Mode | When to refresh |
|---|---|
| Full DOW | `DAY_UNIT` |
| Full date | `DAY_UNIT` |
| Abbr DOW + abbr date | `DAY_UNIT` |
| Empty | `DAY_UNIT` (hides row; no ongoing updates) |
| Steps (default) | `MINUTE_UNIT`; `HealthEventSignificantUpdate`; init; focus resume |
| Steps (live, opt-in) | Also `HealthEventMovementUpdate` when `KEY_LIVE_STEPS` enabled |
| All rows + time | `AppFocusService` `did_focus(true)` |

### Battery / refresh frequency

Each display update costs battery. Default: **minute-level** refresh for steps (matches time/date).

**Opt-in live steps (`KEY_LIVE_STEPS`, default off):** also refresh on `HealthEventMovementUpdate`. Calls `refresh_steps_rows()` only — not a full display redraw.

### Returning from notification — NOT init

`handle_init()` runs once at load (with idempotent re-init via `handle_deinit()` on reload). On notification dismiss, `AppFocusService` `did_focus(true)` refreshes stale UI:

```c
static void app_focus_did_change(bool in_focus) {
  if (!s_app_active || !in_focus) return;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tick_handler(t, MINUTE_UNIT | DAY_UNIT);
  battery_handler(battery_state_service_peek());
  layer_mark_dirty(window_layer);
}
```

Clay settings save via AppMessage — separate path, no focus event needed.

### Lifecycle guards

Handlers check `s_app_active` and bail out during deinit/reload. `handle_deinit()` tears down layers, unsubscribes services, and resets flags before re-init.

---

## Health API integration

In [`package.json`](../package.json):

```json
"capabilities": ["location", "configurable", "health"]
```

```c
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
```

```c
static void health_handler(HealthEventType event, void *context) {
  if (!s_app_active) return;
  bool has_step_row = (flag_topRow == ROW_STEPS || flag_bottomRow == ROW_STEPS);
  if (!has_step_row) return;
  if (event == HealthEventSignificantUpdate) {
    refresh_steps_rows();
  } else if (event == HealthEventMovementUpdate && flag_live_steps) {
    refresh_steps_rows();
  }
}
```

- `aplite`: no `PBL_HEALTH` — show `"--"`
- Omitting `"health"` capability kills the app on health-capable platforms

---

## Settings / persistence

### Message keys

| Key | ID | Purpose |
|---|---|---|
| `KEY_TOP_ROW` | 6 | Top row mode (0–4) |
| `KEY_BOTTOM_ROW` | 8 | Bottom row mode (0–4) |
| `KEY_LIVE_STEPS` | 9 | 0 = minute only, 1 = live |

### Clay — [`src/pkjs/config.json`](../src/pkjs/config.json)

**Display Rows** section: two selects (defaults top=0, bottom=1), five options each including `(empty)`, plus live steps toggle.

### PKJS — [`src/pkjs/app.js`](../src/pkjs/app.js)

Forward and persist new keys in `current_settings`.

Legacy [`html/clean_smart_config.htm`](../html/clean_smart_config.htm): do not update.

### Previewing Clay on arm64 (visual only)

Emulator PKJS is unavailable on arm64 WSL; use Chrome on Windows to review the settings **layout** after editing `config.json`. See [`arm64-pebble-dev.md` — Clay settings visual check](arm64-pebble-dev.md#clay-settings-visual-check):

```bash
rebble build
node docs/tools/clay-preview-url.js
# open build/clay-preview.html in Windows Chrome
```

Full save-to-watch testing still requires `rebble install --phone <ip>` locally, or build/install from [CloudPebble](arm64-pebble-dev.md#optional-cloudpebble-remix) (full PKJS in the browser emulator).

---

## Typography and casing

| Mode | English | Custom Latin | Russian |
|---|---|---|---|
| Full DOW (0) | `strftime %A` | existing `LANG_DAY` | existing ALL CAPS |
| Full date (1) | `strftime %b` | existing `LANG_MONTH` | existing ALL CAPS |
| Abbr DOW + date (3) | `%a` + `%b` uppercase | `LANG_DAY_ABBR` + `LANG_MONTH_UPPER` ALL CAPS | ALL CAPS e.g. `СБ 20-ЯНВ-2026` |
| Empty (4) | — | — | — |

**Font:** only `LANG_RUSSIAN` uses `Big_Noodle_Titling_Cyr.ttf`. Polish uses Latin font but shares wider 6-char month slot with Russian in date formatting.

`LANG_DAY_ABBR[9][7][8]` and `LANG_MONTH_UPPER[9][12][8]` in [`src/c/languages.h`](../src/c/languages.h) (8-byte slots for UTF-8 Cyrillic).

---

## File change summary

| File | Changes |
|---|---|
| [`src/c/languages.h`](../src/c/languages.h) | `LANG_DAY_ABBR`, `LANG_MONTH_UPPER` |
| [`src/c/main.h`](../src/c/main.h) | `RowDisplayMode`, message keys, `STEP_ICON_GAP` |
| [`src/c/main.c`](../src/c/main.c) | Rows, formatters, step icon, health, focus, lifecycle guards |
| [`src/pkjs/config.json`](../src/pkjs/config.json) | Row selects (incl. empty) + live steps |
| [`src/pkjs/app.js`](../src/pkjs/app.js) | Forward/persist keys |
| [`package.json`](../package.json) | `messageKeys`, `health`, platform-specific `ICON_STEPS`, version 2.51.0 |
| [`resources/images/icon_steps_26.png`](../resources/images/icon_steps_26.png) | Rect step icon |
| [`resources/images/icon_steps_19.png`](../resources/images/icon_steps_19.png) | Round (`chalk`) step icon |

No changes to [`Makefile`](../Makefile), [`wscript`](../wscript), or npm dependencies.

---

## Testing checklist

**Done.** All 12 original tests passed during Phase 4.

- [x] Phase 0 baseline build passes
- [x] Default layout matches pre-refactor watchface
- [x] Each row mode on top and bottom (spot-check combinations)
- [x] Steps on `emery`/`diorite`: icon + count; `--` when unavailable
- [x] Live steps off: updates ≤1/min; live steps on: updates while walking
- [x] Steps on `aplite`: `--`, no crash
- [x] Step overflow >99999: `[icon] !!!`
- [x] Language switching in abbr mode (Swedish, Russian, Italian)
- [x] Date format affects mode 1 and mode 3 ordering
- [x] Clay settings persist across reload
- [x] Focus resume after notification refreshes display
- [x] Round (`chalk`): longest strings don't clip

### Post-ship addition

- `(empty)` row option (mode 4) — added after Phase 4; not covered by the original 12 tests.

---

## Risks and mitigations

| Risk | Mitigation | Outcome |
|---|---|---|
| Mode 3 too long on round | DOW ≤3 chars; 3-char months; test Polish/Catalan/Russian | Verified on `chalk` (test 12) |
| Polish uses Latin, not Cyrillic | Only Russian loads Cyrillic font; verify Polish diacritics in abbr tables | Verified in language tests |
| Polish/Russian wider month field | Reuse 6-char month slot logic from full date | Shipped as 8-byte UTF-8 slots |
| ALL CAPS accented chars | Explicit upper tables, not runtime `toupper` | Shipped in `LANG_*_UPPER` tables |
| Mode 3 vs mode 1 confusion | Separate formatters and Clay labels | Shipped |
| Step icon + 5 digits on round | Center icon+text group; 8 px gap | Shipped; `layout_step_row()` centers group |
| Step count >99999 | Show `!!!` | Verified (test 7) |
| Missing `health` capability | Added before any Health API call | Shipped in `package.json` |
| Steps unavailable | Show `--`; `PBL_HEALTH` guards | Verified on `aplite` (test 6) |
| Live steps battery drain | Default off; MovementUpdate redraws step rows only | Verified (test 5) |
| Reload/deinit races | `s_app_active` guards in handlers | Shipped |
