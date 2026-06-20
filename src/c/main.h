#include <pebble.h>
#pragma once

#define KEY_WEATHER_CODE 0
#define KEY_WEATHER_TEMP 1
#define KEY_JSREADY 2

#define KEY_TEMPERATURE_FORMAT 3
#define KEY_HOURS_MINUTES_SEPARATOR 4
#define KEY_DATE_FORMAT 5
#define KEY_TOP_ROW 6
#define KEY_BOTTOM_ROW 8
#define KEY_LIVE_STEPS 9
#define KEY_BLUETOOTH_ALERT 7
#define KEY_LANGUAGE 10
#define KEY_TEXT_COLOR 11
#define KEY_BG_COLOR 12

#ifdef PBL_RECT
#define ICON_WIDTH 40
#define ICON_HEIGHT 20
#else
#define ICON_WIDTH 36
#define ICON_HEIGHT 18
#endif

#define BLUETOOTH_ALERT_DISABLED 0
#define BLUETOOTH_ALERT_SILENT 1
#define BLUETOOTH_ALERT_WEAK 2
#define BLUETOOTH_ALERT_NORMAL 3
#define BLUETOOTH_ALERT_STRONG 4
#define BLUETOOTH_ALERT_DOUBLE 5

typedef enum {
  ROW_FULL_DOW = 0,
  ROW_FULL_DATE = 1,
  ROW_STEPS = 2,
  ROW_ABBR_DOW_DATE = 3,
} RowDisplayMode;

#define STEP_ICON_SIZE 18
#define STEP_ICON_GAP 4

#define ROW_TEXT_BUF_SIZE 48
#define ROW_DOW_ABBR_MAX 3
#define ROW_MONTH_ABBR_MAX 6

// define macro comparing PBL_DISPLAY_HEIGHT with 168
#ifdef PBL_RECT
#if PBL_DISPLAY_HEIGHT == 168
  #define PBL_IF_HEIGHT_168_ELSE(expr_if_true, expr_if_false) (expr_if_true)
#else
  #define PBL_IF_HEIGHT_168_ELSE(expr_if_true, expr_if_false) (expr_if_false)
#endif
#else
#define PBL_IF_HEIGHT_168_ELSE(expr_if_true, expr_if_false) (expr_if_true)
#endif

// bluetooth vibe patterns
const VibePattern VIBE_PATTERN_WEAK = {
	.durations = (uint32_t[]){100},
	.num_segments = 1};

const VibePattern VIBE_PATTERN_NORMAL = {
	.durations = (uint32_t[]){300},
	.num_segments = 1};

const VibePattern VIBE_PATTERN_STRONG = {
	.durations = (uint32_t[]){500},
	.num_segments = 1};

const VibePattern VIBE_PATTERN_DOUBLE = {
	.durations = (uint32_t[]){500, 100, 500},
	.num_segments = 3};
