#include <pebble.h>
#pragma once

#define KEY_WEATHER_CODE 0
#define KEY_WEATHER_TEMP 1
#define KEY_JSREADY 2  
  
#define KEY_TEMPERATURE_FORMAT 3
#define KEY_HOURS_MINUTES_SEPARATOR 4
#define KEY_DATE_FORMAT 5
#define KEY_INVERT_COLORS 6
#define KEY_BLUETOOTH_ALERT 7
#define KEY_LOCATION_SERVICE 8  
#define KEY_WEATHER_INTERVAL 9  
#define KEY_LANGUAGE 10
  
#define KEY_LOCATION_AUTOMATIC  0  
#define KEY_LOCATION_MANUAL  1
#define KEY_LOCATION_DISABLED  2

#ifdef PBL_RECT
  #define ICON_WIDTH  40
  #define ICON_HEIGHT 20
#else  
  #define ICON_WIDTH  36
  #define ICON_HEIGHT 18
#endif

#define BLUETOOTH_ALERT_DISABLED 0  
#define BLUETOOTH_ALERT_SILENT 1
#define BLUETOOTH_ALERT_WEAK 2
#define BLUETOOTH_ALERT_NORMAL 3
#define BLUETOOTH_ALERT_STRONG 4
#define BLUETOOTH_ALERT_DOUBLE 5

// bluetooth vibe patterns
const VibePattern VIBE_PATTERN_WEAK = {
	.durations = (uint32_t []) {100},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_NORMAL = {
	.durations = (uint32_t []) {300},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_STRONG = {
	.durations = (uint32_t []) {500},
	.num_segments = 1
};

const VibePattern VIBE_PATTERN_DOUBLE = {
	.durations = (uint32_t []) {500,100,500},
	.num_segments = 3
};