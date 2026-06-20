#include <pebble.h>
#include <ctype.h>
#include "main.h"
#include <pebble-effect-layer/pebble-effect-layer.h>
#include "languages.h"

Window *my_window;
Layer *window_layer;

TextLayer *text_time, *text_row_top, *text_row_bottom, *text_battery, *text_temp;
Layer *graphics_layer;
BitmapLayer *temp_layer;
Layer *step_icon_top, *step_icon_bottom;
GBitmap *meteoicons_all, *meteoicon_current, *steps_icon_bitmap;

GFont bn_69, bn_30, bn_26, bn_20, bn_19;

char s_row_top[ROW_TEXT_BUF_SIZE];
char s_row_bottom[ROW_TEXT_BUF_SIZE];
char s_time[] = "88.44mm";
char s_battery[] = "100%";            // test
char s_temp[] = "-100°";

EffectLayer *zoom_layer_time, *zoom_layer_meteoicon;

uint8_t flag_hoursMinutesSeparator, flag_dateFormat, flag_bluetooth_alert, flag_language;
uint8_t flag_topRow, flag_bottomRow, flag_live_steps;
int flag_textColor, flag_bgColor;
bool flag_messaging_is_busy = false, flag_js_is_ready = false;

GRect bounds;
GPoint center;
GRect row_top_frame, row_bottom_frame;
GTextAlignment row_bottom_text_align;

static GRect get_time_frame()
{
#ifdef PBL_RECT
  return GRect(0, 53 * PBL_DISPLAY_HEIGHT / 168 + PBL_IF_HEIGHT_168_ELSE(0, 18), bounds.size.w, 70 * PBL_DISPLAY_HEIGHT / 168);
#else
  return GRect(0, 38, bounds.size.w, 70);
#endif
}

static void set_time_frame(GRect frame)
{
  layer_set_frame(text_layer_get_layer(text_time), frame);

  if (zoom_layer_time)
  {
    effect_layer_set_frame(zoom_layer_time, frame);
  }
}

static void set_time_frame_for_unobstructed_area(GRect free_area)
{
  GRect frame = get_time_frame();
  int16_t obstruction_height = bounds.size.h - free_area.size.h;

  if (obstruction_height < 0)
  {
    obstruction_height = 0;
  }

  frame.origin.y -= obstruction_height * 15 / 100;
  set_time_frame(frame);
}

// // {*********************** THIS BLOCK PROPERLY RESTORES EFFECT LAYER AFTER A NOTIFICATION IS DISMISSED

// // when app got focus - restore and refresh window - that makes it dynamic again
// static void app_focus_changed(bool focused) {
//   if (focused && effect_layer) {
//      layer_set_hidden(window_layer, false);
//      layer_mark_dirty(window_layer);
//   }

// }

// // when app is about to regain focus - hide main window - this restores static pic of previous screen appear
// static void app_focus_changing(bool focused) {
//   if (focused && effect_layer) {
//      layer_set_hidden(window_layer, true);
//   }

// }
// // *********************** }

// calling for weather update
static void update_weather()
{
  // Only grab the weather if we can talk to phone AND weather is enabled AND currently message is not being processed and JS on phone is ready
  if (bluetooth_connection_service_peek() && !flag_messaging_is_busy && flag_js_is_ready)
  {
    // APP_LOG(APP_LOG_LEVEL_INFO, "**** I am inside 'update_weather()' about to request weather from the phone ***");

    // need to have some data - sending dummy
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    Tuplet dictionary[] = {
        TupletInteger(0, 0),
    };
    dict_write_tuplet(iter, &dictionary[0]);

    flag_messaging_is_busy = true;
    app_message_outbox_send();
  }
}

// showing temp
static void show_temperature(int w_current)
{
  // APP_LOG(APP_LOG_LEVEL_INFO, "**** I am inside 'show_temperature()'; TEMP in Pebble: %d", w_current);
  static char buffer[6];
  snprintf(buffer, sizeof(buffer), "%i\u00B0", w_current);
  text_layer_set_text(text_temp, buffer);
}

// showing weather icon
static void show_icon(int w_icon)
{
  if (meteoicon_current)
    gbitmap_destroy(meteoicon_current);
  meteoicon_current = gbitmap_create_as_sub_bitmap(meteoicons_all, GRect(0, ICON_HEIGHT * w_icon, ICON_WIDTH, ICON_HEIGHT));
  bitmap_layer_set_bitmap(temp_layer, meteoicon_current);
}

static void tint_meteoicon() {
  GColor *palette = gbitmap_get_palette(meteoicons_all);
  if (!palette) return;

  int num_colors;
  switch (gbitmap_get_format(meteoicons_all)) {
    case GBitmapFormat1BitPalette: num_colors = 2; break;
    case GBitmapFormat2BitPalette: num_colors = 4; break;
    case GBitmapFormat4BitPalette: num_colors = 16; break;
    default: return;
  }

  GColor new_color = GColorFromHEX(flag_textColor);
  for (int i = 0; i < num_colors; i++) {
    if (!gcolor_equal(palette[i], GColorClear)) {
      palette[i] = new_color;
    }
  }
  layer_mark_dirty(bitmap_layer_get_layer(temp_layer));
}

// tint footsteps icon to match KEY_TEXT_COLOR
static void tint_step_icon(void)
{
  if (!steps_icon_bitmap) return;

  GColor *palette = gbitmap_get_palette(steps_icon_bitmap);
  if (!palette) return;

  int num_colors;
  switch (gbitmap_get_format(steps_icon_bitmap))
  {
    case GBitmapFormat1BitPalette: num_colors = 2; break;
    case GBitmapFormat2BitPalette: num_colors = 4; break;
    case GBitmapFormat4BitPalette: num_colors = 16; break;
    default: return;
  }

  GColor new_color = GColorFromHEX(flag_textColor);
  for (int i = 0; i < num_colors; i++)
  {
    if (!gcolor_equal(palette[i], GColorClear))
    {
      palette[i] = new_color;
    }
  }
  if (step_icon_top) layer_mark_dirty(step_icon_top);
  if (step_icon_bottom) layer_mark_dirty(step_icon_bottom);
}

static void step_icon_layer_update_proc(Layer *layer, GContext *ctx)
{
  if (!steps_icon_bitmap) return;

  GRect bounds = layer_get_bounds(layer);
  GSize bmp_size = gbitmap_get_bounds(steps_icon_bitmap).size;
  GRect dest = GRect((bounds.size.w - bmp_size.w) / 2,
                     (bounds.size.h - bmp_size.h) / 2,
                     bmp_size.w, bmp_size.h);

  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, steps_icon_bitmap, dest);
}

// configurable top/bottom rows (flag_topRow / flag_bottomRow); center time row unchanged
static void format_full_date(char *buf, size_t len, struct tm *tick_time)
{
  switch (flag_dateFormat)
  {
  case 0:
    if (flag_language == LANG_RUSSIAN || flag_language == LANG_POLISH)
    {
      strftime(buf, len, "%b   -%d-%Y", tick_time);
      strncpy(&buf[0], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
    }
    else
    {
      strftime(buf, len, "%b-%d-%Y", tick_time);
      if (flag_language != LANG_DEFAULT)
      {
        strncpy(&buf[0], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
      }
    }
    break;
  case 1:
    if (flag_language == LANG_RUSSIAN)
    {
      strftime(buf, len, "%d-%b   -%Y", tick_time);
      strncpy(&buf[3], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
    }
    else
    {
      strftime(buf, len, "%d-%b-%Y", tick_time);
      if (flag_language != LANG_DEFAULT)
      {
        strncpy(&buf[3], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
      }
    }
    break;
  case 2:
    strftime(buf, len, "%Y-%m-%d", tick_time);
    break;
  }
}

static void format_full_dow(char *buf, size_t len, struct tm *tick_time)
{
  if (len == 0) return;

  if (flag_language != LANG_DEFAULT)
  {
    strncpy(buf, LANG_DAY[flag_language][tick_time->tm_wday], len - 1);
  }
  else
  {
    strftime(buf, len, "%A", tick_time);
  }
  buf[len - 1] = '\0';
}

static void get_abbr_dow(char *buf, size_t len, struct tm *tick_time)
{
  if (len == 0) return;

  if (flag_language != LANG_DEFAULT)
  {
    strncpy(buf, LANG_DAY_ABBR[flag_language][tick_time->tm_wday], len - 1);
  }
  else
  {
    strftime(buf, len, "%a", tick_time);
    for (char *p = buf; *p; p++)
    {
      *p = toupper((unsigned char)*p);
    }
  }
  buf[len - 1] = '\0';
}

static void get_abbr_month(char *buf, size_t len, struct tm *tick_time)
{
  if (len == 0) return;

  if (flag_language != LANG_DEFAULT)
  {
    strncpy(buf, LANG_MONTH_UPPER[flag_language][tick_time->tm_mon], len - 1);
  }
  else
  {
    strftime(buf, len, "%b", tick_time);
    for (char *p = buf; *p; p++)
    {
      *p = toupper((unsigned char)*p);
    }
  }
  buf[len - 1] = '\0';
}

// mode 3: abbreviated DOW + abbreviated date, ordered by KEY_DATE_FORMAT
static void format_abbr_dow_date(char *buf, size_t len, struct tm *tick_time)
{
  char dow[ROW_DOW_ABBR_MAX + 1];
  char mon[ROW_MONTH_ABBR_MAX + 1];
  const int year = tick_time->tm_year + 1900;

  get_abbr_dow(dow, sizeof(dow), tick_time);
  get_abbr_month(mon, sizeof(mon), tick_time);

  switch (flag_dateFormat)
  {
  case 0:
    snprintf(buf, ROW_TEXT_BUF_SIZE, "%.*s %.*s-%02d-%04d",
             ROW_DOW_ABBR_MAX, dow, ROW_MONTH_ABBR_MAX, mon, tick_time->tm_mday, year);
    break;
  case 1:
    snprintf(buf, ROW_TEXT_BUF_SIZE, "%.*s %02d-%.*s-%04d",
             ROW_DOW_ABBR_MAX, dow, tick_time->tm_mday, ROW_MONTH_ABBR_MAX, mon, year);
    break;
  case 2:
    snprintf(buf, ROW_TEXT_BUF_SIZE, "%.*s %04d-%02d-%02d",
             ROW_DOW_ABBR_MAX, dow, year, tick_time->tm_mon + 1, tick_time->tm_mday);
    break;
  }
  if (len < ROW_TEXT_BUF_SIZE)
  {
    buf[len - 1] = '\0';
  }
}

// step count from Health API; "--" if unavailable (aplite / no permission), "!!!" if >99999
static void format_steps(char *buf, size_t len)
{
#if defined(PBL_HEALTH)
  time_t now = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(
      HealthMetricStepCount, now - SECONDS_PER_DAY, now);
  if (!(mask & HealthServiceAccessibilityMaskAvailable))
  {
    strncpy(buf, "--", ROW_TEXT_BUF_SIZE);
    if (len < ROW_TEXT_BUF_SIZE)
    {
      buf[len - 1] = '\0';
    }
    return;
  }

  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  if (steps > 99999)
  {
    strncpy(buf, "!!!", ROW_TEXT_BUF_SIZE);
  }
  else
  {
    snprintf(buf, ROW_TEXT_BUF_SIZE, "%u", (unsigned int)steps);
  }
  if (len < ROW_TEXT_BUF_SIZE)
  {
    buf[len - 1] = '\0';
  }
#else
  strncpy(buf, "--", ROW_TEXT_BUF_SIZE);
  if (len < ROW_TEXT_BUF_SIZE)
  {
    buf[len - 1] = '\0';
  }
#endif
}

// icon height matches the row font size (Big Noodle nominal pt)
static int step_icon_line_height(TextLayer *text)
{
  if (text == text_row_top)
  {
#ifdef PBL_RECT
    return PBL_IF_HEIGHT_168_ELSE(30, 41);
#else
    return 20;
#endif
  }
  if (text == text_row_bottom)
  {
#ifdef PBL_RECT
    return PBL_IF_HEIGHT_168_ELSE(26, 35);
#else
    return 19;
#endif
  }
  return 18;
}

// step row: icon + gap + text, centered as one group within the row bounds
static void layout_step_row(TextLayer *text, Layer *icon, GRect full_frame, const char *text_str)
{
  text_layer_set_text(text, text_str);
  text_layer_set_text_alignment(text, GTextAlignmentLeft);

  GSize content = text_layer_get_content_size(text);
  int icon_size = step_icon_line_height(text);
  if (icon_size > full_frame.size.h)
  {
    icon_size = full_frame.size.h;
  }

  int total_w = icon_size + STEP_ICON_GAP + content.w;
  int start_x = full_frame.origin.x + (full_frame.size.w - total_w) / 2;
  int icon_y = full_frame.origin.y + (full_frame.size.h - icon_size) / 2;

  layer_set_frame(icon, GRect(start_x, icon_y, icon_size, icon_size));
  layer_set_hidden(icon, false);

  layer_set_frame(text_layer_get_layer(text),
                  GRect(start_x + icon_size + STEP_ICON_GAP, full_frame.origin.y,
                        content.w, full_frame.size.h));
}

static void layout_text_row(TextLayer *text, Layer *icon, GRect full_frame,
                            GTextAlignment align, const char *text_str)
{
  layer_set_hidden(icon, true);
  layer_set_frame(text_layer_get_layer(text), full_frame);
  text_layer_set_text_alignment(text, align);
  text_layer_set_text(text, text_str);
}

static void update_row(TextLayer *text, Layer *icon, GRect full_frame,
                       GTextAlignment text_align, RowDisplayMode mode, struct tm *tick_time,
                       char *buf, size_t buf_len)
{
  switch (mode)
  {
  case ROW_FULL_DOW:
    format_full_dow(buf, buf_len, tick_time);
    layout_text_row(text, icon, full_frame, text_align, buf);
    break;
  case ROW_FULL_DATE:
    format_full_date(buf, buf_len, tick_time);
    layout_text_row(text, icon, full_frame, text_align, buf);
    break;
  case ROW_STEPS:
    format_steps(buf, buf_len);
    layout_step_row(text, icon, full_frame, buf);
    break;
  case ROW_ABBR_DOW_DATE:
    format_abbr_dow_date(buf, buf_len, tick_time);
    layout_text_row(text, icon, full_frame, text_align, buf);
    break;
  }
}

// step rows only — used on MINUTE_UNIT tick and from health/focus handlers
static void refresh_steps_rows(void)
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  if (flag_topRow == ROW_STEPS)
  {
    update_row(text_row_top, step_icon_top, row_top_frame, GTextAlignmentCenter,
               ROW_STEPS, t, s_row_top, ROW_TEXT_BUF_SIZE);
  }
  if (flag_bottomRow == ROW_STEPS)
  {
    update_row(text_row_bottom, step_icon_bottom, row_bottom_frame, row_bottom_text_align,
               ROW_STEPS, t, s_row_bottom, ROW_TEXT_BUF_SIZE);
  }
}

// called from tick_handler — DAY_UNIT for text rows + midnight step reset; MINUTE_UNIT for step ticks
static void refresh_rows(struct tm *tick_time, TimeUnits units_changed)
{
  if (units_changed & DAY_UNIT)
  {
    if (flag_topRow != ROW_STEPS)
    {
      update_row(text_row_top, step_icon_top, row_top_frame, GTextAlignmentCenter,
                 (RowDisplayMode)flag_topRow, tick_time, s_row_top, ROW_TEXT_BUF_SIZE);
    }
    if (flag_bottomRow != ROW_STEPS)
    {
      update_row(text_row_bottom, step_icon_bottom, row_bottom_frame, row_bottom_text_align,
                 (RowDisplayMode)flag_bottomRow, tick_time, s_row_bottom, ROW_TEXT_BUF_SIZE);
    }
    // step count resets at midnight — refresh even though steps normally tick on MINUTE_UNIT
    refresh_steps_rows();
  }

  if (units_changed & MINUTE_UNIT)
  {
    refresh_steps_rows();
  }
}

#if defined(PBL_HEALTH)
// SignificantUpdate always; MovementUpdate only when KEY_LIVE_STEPS is on
static void health_handler(HealthEventType event, void *context)
{
  bool has_step_row = (flag_topRow == ROW_STEPS || flag_bottomRow == ROW_STEPS);
  if (!has_step_row) return;

  if (event == HealthEventSignificantUpdate)
  {
    refresh_steps_rows();
  }
  else if (event == HealthEventMovementUpdate && flag_live_steps)
  {
    refresh_steps_rows();
  }
}
#endif

// handling time
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{

  char format[6];

  // building format 12h/24h
  if (clock_is_24h_style())
  {
    strcpy(format, "%H:%M"); // e.g "14:46"
  }
  else
  {
    strcpy(format, "%l:%M"); // e.g " 2:46" -- with leading space
  }

  // if separator is dot = replacing colon with it
  if (flag_hoursMinutesSeparator == 1)
    format[2] = '.';

  if (units_changed & MINUTE_UNIT)
  { // on minutes change - change time

    strftime(s_time, sizeof(s_time), format, tick_time);

    if (s_time[0] == ' ')
    { // if in 12h mode we have leading space in time - don't display it (it will screw centering of text) start with next char
      text_layer_set_text(text_time, &s_time[1]);
    }
    else
    {
      text_layer_set_text(text_time, s_time);
    }

    if (!(tick_time->tm_min % 60) && (tick_time->tm_sec == 0))
    { // on configured weather interval change - update the weather
      // APP_LOG(APP_LOG_LEVEL_INFO, "**** I am inside 'tick_handler()' about to call 'update_weather();' at minute %d min on %d interval", tick_time->tm_min, flag_weatherInterval);
      update_weather();
    }
  }

  if (units_changed & DAY_UNIT)
  { // on day change - refresh DOW, date, abbr rows, and step count (midnight reset)
    refresh_rows(tick_time, DAY_UNIT);
  }

  if (units_changed & MINUTE_UNIT)
  { // on minute change - refresh step rows (live steps also via health events)
    refresh_rows(tick_time, MINUTE_UNIT);
  }
}

void load_fonts()
{

  fonts_unload_custom_font(bn_69);
  fonts_unload_custom_font(bn_19);

  if (flag_language == LANG_RUSSIAN)
  {
    bn_69 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_69));
    bn_19 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_19, RESOURCE_ID_BIG_NOODLE_26)));
  }
  else
  {
    bn_69 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_ENG_69));
    bn_19 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_ENG_19, RESOURCE_ID_BIG_NOODLE_ENG_26)));
  }

#ifdef PBL_RECT
  fonts_unload_custom_font(bn_30);
  fonts_unload_custom_font(bn_26);

  if (flag_language == LANG_RUSSIAN)
  {
    bn_30 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_30, RESOURCE_ID_BIG_NOODLE_41)));
    bn_26 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_26, RESOURCE_ID_BIG_NOODLE_35)));
  } 
  else
  {
    bn_30 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_ENG_30, RESOURCE_ID_BIG_NOODLE_ENG_41)));
    bn_26 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_ENG_26, RESOURCE_ID_BIG_NOODLE_ENG_35)));
  }

#else
  fonts_unload_custom_font(bn_20);

  if (flag_language == LANG_RUSSIAN)
  {
    bn_20 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_26, RESOURCE_ID_BIG_NOODLE_ENG_35)));
  }
  else
  {
    bn_20 = fonts_load_custom_font(resource_get_handle(PBL_IF_HEIGHT_168_ELSE(RESOURCE_ID_BIG_NOODLE_ENG_26, RESOURCE_ID_BIG_NOODLE_ENG_35)));
  }
#endif
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
  // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' Message from the phone received!");

  // Read first item
  Tuple *t = dict_read_first(iterator);

  bool need_weather = 0;
  bool need_time = 0;

  // For all items
  while (t != NULL)
  {
    // Which key was received?
    switch (t->key)
    {

    // weather data keys
    case KEY_WEATHER_TEMP:
      persist_write_int(KEY_WEATHER_TEMP, t->value->int32);
      show_temperature(t->value->int32);
      break;
    case KEY_WEATHER_CODE:
      persist_write_int(KEY_WEATHER_CODE, t->value->int32);
      show_icon(t->value->int32);
      break;
    case KEY_JSREADY:
      // JS ready lets get the weather
      if (t->value->int16)
      {
        // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' message 'JS is ready' received !");
        flag_js_is_ready = true;
        need_weather = 1;
      }
      break;

      // config keys
    case KEY_TEMPERATURE_FORMAT: // if temp format changed from F to C or back - need re-request weather
      // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' switching temp format");
      need_weather = 1;
      break;
    case KEY_HOURS_MINUTES_SEPARATOR:
      if (t->value->int32 != flag_hoursMinutesSeparator)
      {
        persist_write_int(KEY_HOURS_MINUTES_SEPARATOR, t->value->int32);
        flag_hoursMinutesSeparator = t->value->int32;
        need_time = 1;
      }
      break;
    case KEY_DATE_FORMAT:
      if (t->value->int32 != flag_dateFormat)
      {
        persist_write_int(KEY_DATE_FORMAT, t->value->int32);
        flag_dateFormat = t->value->int32;
        need_time = 1;
      }
      break;
    case KEY_BLUETOOTH_ALERT:
      if (flag_bluetooth_alert != t->value->uint8)
      {
        persist_write_int(KEY_BLUETOOTH_ALERT, t->value->uint8);
        flag_bluetooth_alert = t->value->uint8;
        layer_mark_dirty(graphics_layer);
      }
      break;
    case KEY_LANGUAGE:
      if (t->value->int32 != flag_language)
      {
        persist_write_int(KEY_LANGUAGE, t->value->int32);
        flag_language = t->value->int32;
        load_fonts();
        need_time = 1;
      }
      break;
    case KEY_TEXT_COLOR:
      if (t->value->int32 != flag_textColor)
      {
        persist_write_int(KEY_TEXT_COLOR, t->value->int32);
        flag_textColor = t->value->int32;
        tint_meteoicon();
        tint_step_icon();
        text_layer_set_text_color(text_time,        GColorFromHEX(flag_textColor));
        text_layer_set_text_color(text_row_top,     GColorFromHEX(flag_textColor));
        text_layer_set_text_color(text_row_bottom,  GColorFromHEX(flag_textColor));
        text_layer_set_text_color(text_battery,     GColorFromHEX(flag_textColor));
        text_layer_set_text_color(text_temp,        GColorFromHEX(flag_textColor));
      }
      break;
    case KEY_BG_COLOR:
      if (t->value->int32 != flag_bgColor)
      {
        persist_write_int(KEY_BG_COLOR, t->value->int32);
        flag_bgColor = t->value->int32;
        window_set_background_color(my_window, GColorFromHEX(flag_bgColor));
      }
      break;
    case KEY_TOP_ROW:
      if (t->value->int32 != flag_topRow)
      {
        persist_write_int(KEY_TOP_ROW, t->value->int32);
        flag_topRow = t->value->int32;
        need_time = 1;
      }
      break;
    case KEY_BOTTOM_ROW:
      if (t->value->int32 != flag_bottomRow)
      {
        persist_write_int(KEY_BOTTOM_ROW, t->value->int32);
        flag_bottomRow = t->value->int32;
        need_time = 1;
      }
      break;
    case KEY_LIVE_STEPS:
      if (t->value->int32 != flag_live_steps)
      {
        persist_write_int(KEY_LIVE_STEPS, t->value->int32);
        flag_live_steps = t->value->int32;
      }
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }

  if (need_weather)
  {
    // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' about to call 'update_weather();");
    update_weather();
  }

  if (need_time)
  {
    // Get a time structure
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);

    // Manually call the tick handler
    tick_handler(t, MINUTE_UNIT | DAY_UNIT);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  // APP_LOG(APP_LOG_LEVEL_ERROR, "____Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  flag_messaging_is_busy = false;
  // APP_LOG(APP_LOG_LEVEL_ERROR, "____Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  flag_messaging_is_busy = false;
  // APP_LOG(APP_LOG_LEVEL_INFO, "_____Outbox send success!");
}

// creates text layer at given coordinates, given font and alignment
TextLayer *create_text_layer(GRect coords, GFont font, GTextAlignment align)
{
  TextLayer *text_layer = text_layer_create(coords);
  text_layer_set_font(text_layer, font);
  text_layer_set_text_color(text_layer, GColorFromHEX(flag_textColor));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_alignment(text_layer, align);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  return text_layer;
}

static void bluetooth_handler(bool state)
{

  if (state)
  {
    // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'bluetooth_handler()' about to call 'update_weather();");
    update_weather();
  }

  // if Bluetooth alert is totally disabled - exit from here
  if (flag_bluetooth_alert == BLUETOOTH_ALERT_DISABLED)
    return;

  switch (flag_bluetooth_alert)
  {
  case BLUETOOTH_ALERT_WEAK:
    vibes_enqueue_custom_pattern(VIBE_PATTERN_WEAK);
    break;
  case BLUETOOTH_ALERT_NORMAL:
    vibes_enqueue_custom_pattern(VIBE_PATTERN_NORMAL);
    break;
  case BLUETOOTH_ALERT_STRONG:
    vibes_enqueue_custom_pattern(VIBE_PATTERN_STRONG);
    break;
  case BLUETOOTH_ALERT_DOUBLE:
    vibes_enqueue_custom_pattern(VIBE_PATTERN_DOUBLE);
    break;
  }

  layer_mark_dirty(graphics_layer);
}

static void graphics_update_proc(Layer *layer, GContext *ctx)
{

  static GColor color;

#ifdef PBL_COLOR

  // doing battery color in ranges with fall thru:
  //       100% - 50% - GColorGreen
  //       49% - 20% - GColorIcterine
  //       19% - 0% - GColorRed

  switch (battery_state_service_peek().charge_percent)
  {
  case 100:
  case 90:
  case 80:
  case 70:
  case 60:
  case 50:
    color = GColorGreen;
    break;
  case 40:
  case 30:
  case 20:
    color = GColorIcterine;
    break;
  case 10:
  case 0:
    color = GColorRed;
    break;
  }
#else
  color = GColorWhite;
#endif

#ifdef PBL_RECT // on Aplite & Basalt draw think line for battery
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, GRect(0, 25 * PBL_DISPLAY_HEIGHT / 168, PBL_DISPLAY_WIDTH, 3), 0, GCornersAll);
#else // on Chalk draw think circle
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_circle(ctx, center, 85);
#endif

  if (flag_bluetooth_alert != BLUETOOTH_ALERT_DISABLED && bluetooth_connection_service_peek())
  { // checkin bluetooth only if check is enabled
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorCyan);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif

#ifdef PBL_RECT // on Aplite & Basalt draw thick line
    graphics_fill_rect(ctx, GRect(0, PBL_DISPLAY_HEIGHT - 3, PBL_DISPLAY_WIDTH, 3), 0, GCornersAll);
#else // on Chalk draw think circle
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_draw_circle(ctx, center, 76);
#endif
  }
}

static void battery_handler(BatteryChargeState state)
{
  snprintf(s_battery, sizeof("100%"), "%d%%", state.charge_percent);
  text_layer_set_text(text_battery, s_battery);

#ifndef PBL_RECT
  static GColor color;
  // doing battery color in ranges with fall thru:
  //       100% - 50% - GColorGreen
  //       49% - 20% - GColorIcterine
  //       19% - 0% - GColorRed
  switch (state.charge_percent)
  {
  case 100:
  case 90:
  case 80:
  case 70:
  case 60:
  case 50:
    color = GColorGreen;
    break;
  case 40:
  case 30:
  case 20:
    color = GColorIcterine;
    break;
  case 10:
  case 0:
    color = GColorRed;
    break;
  }

  text_layer_set_text_color(text_battery, color);
#endif
}

// watchface regains focus after notification — refresh without re-running handle_init()
static void app_focus_did_change(bool in_focus)
{
  if (!in_focus) return;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tick_handler(t, MINUTE_UNIT | DAY_UNIT);
  battery_handler(battery_state_service_peek());
  layer_mark_dirty(window_layer);
}

// adjusting time location when timeline quickview shows.
void unobstructed_changed(AnimationProgress progress, void *context)
{
  set_time_frame_for_unobstructed_area(layer_get_unobstructed_bounds(window_layer));
}

void unobstructed_did_change(void *context)
{
  set_time_frame_for_unobstructed_area(layer_get_unobstructed_bounds(window_layer));
}

void handle_init(void)
{

  //   app_focus_service_subscribe_handlers((AppFocusHandlers){
  //     .did_focus = app_focus_changed,
  //     .will_focus = app_focus_changing
  //   });

  // going international
  setlocale(LC_ALL, "");

  my_window = window_create();
  window_stack_push(my_window, true);

  window_layer = window_get_root_layer(my_window);
  bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&bounds);

  graphics_layer = layer_create(bounds);
  layer_set_update_proc(graphics_layer, graphics_update_proc);
  layer_add_child(window_layer, graphics_layer);

  meteoicons_all = gbitmap_create_with_resource(RESOURCE_ID_METEOICONS);
#ifdef PBL_RECT
  temp_layer = bitmap_layer_create(GRect(51 * PBL_DISPLAY_WIDTH / 144, 1, 41 * PBL_DISPLAY_WIDTH / 144, 20 * PBL_DISPLAY_HEIGHT / 168));
#else
  temp_layer = bitmap_layer_create(GRect(86, 137, 41, 21));
#endif
  bitmap_layer_set_compositing_mode(temp_layer, GCompOpSet);
  layer_add_child(graphics_layer, bitmap_layer_get_layer(temp_layer));

  flag_hoursMinutesSeparator = persist_exists(KEY_HOURS_MINUTES_SEPARATOR) ? persist_read_int(KEY_HOURS_MINUTES_SEPARATOR) : 0;
  flag_dateFormat = persist_exists(KEY_DATE_FORMAT) ? persist_read_int(KEY_DATE_FORMAT) : 0;
  flag_topRow = persist_exists(KEY_TOP_ROW) ? persist_read_int(KEY_TOP_ROW) : 0;
  flag_bottomRow = persist_exists(KEY_BOTTOM_ROW) ? persist_read_int(KEY_BOTTOM_ROW) : 1;
  flag_live_steps = persist_exists(KEY_LIVE_STEPS) ? persist_read_int(KEY_LIVE_STEPS) : 0;
  flag_bluetooth_alert = persist_exists(KEY_BLUETOOTH_ALERT) ? persist_read_int(KEY_BLUETOOTH_ALERT) : 0;
  flag_language = persist_exists(KEY_LANGUAGE) ? persist_read_int(KEY_LANGUAGE) : LANG_DEFAULT;
  flag_textColor = persist_exists(KEY_TEXT_COLOR) ? persist_read_int(KEY_TEXT_COLOR) : 0xFFFFFF;
  flag_bgColor   = persist_exists(KEY_BG_COLOR)   ? persist_read_int(KEY_BG_COLOR)   : 0x000000;
  window_set_background_color(my_window, GColorFromHEX(flag_bgColor));
  tint_meteoicon();

  load_fonts();

  steps_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_STEPS);
  tint_step_icon();

#ifdef PBL_RECT
  row_top_frame = GRect(0, 30 * PBL_DISPLAY_HEIGHT / 168, bounds.size.w, 31 * PBL_DISPLAY_HEIGHT / 168);
  row_bottom_text_align = GTextAlignmentCenter;
  text_row_top = create_text_layer(row_top_frame, bn_30, GTextAlignmentCenter);
  text_time = create_text_layer(get_time_frame(), bn_69, GTextAlignmentCenter);
  row_bottom_frame = GRect(0, 129 * PBL_DISPLAY_HEIGHT / 168, bounds.size.w, 27 * PBL_DISPLAY_HEIGHT / 168);
  text_row_bottom = create_text_layer(row_bottom_frame, bn_26, GTextAlignmentCenter);
  text_battery = create_text_layer(GRect(PBL_DISPLAY_WIDTH - 46 * PBL_DISPLAY_WIDTH / 144, 0, 43 * PBL_DISPLAY_WIDTH / 144, 21 * PBL_DISPLAY_HEIGHT / 168), bn_19, GTextAlignmentRight);
  text_temp = create_text_layer(GRect(3, 0, 80 * PBL_DISPLAY_WIDTH / 144, 21 * PBL_DISPLAY_HEIGHT / 168), bn_19, GTextAlignmentLeft);

  #if PBL_DISPLAY_HEIGHT != 168
  zoom_layer_time = effect_layer_create(get_time_frame());
  effect_layer_add_effect(zoom_layer_time, effect_zoom, EL_ZOOM(139, 136)); 
  layer_add_child(window_layer, effect_layer_get_layer(zoom_layer_time));

  zoom_layer_meteoicon = effect_layer_create(GRect(51 * PBL_DISPLAY_WIDTH / 144, 1, 41 * PBL_DISPLAY_WIDTH / 144, 20 * PBL_DISPLAY_HEIGHT / 168));
  effect_layer_add_effect(zoom_layer_meteoicon, effect_zoom, EL_ZOOM(139, 136)); 
  layer_add_child(window_layer, effect_layer_get_layer(zoom_layer_meteoicon));
  #endif
#else
  row_top_frame = GRect(0, 23, bounds.size.w, 31);
  row_bottom_text_align = GTextAlignmentLeft;
  text_row_top = create_text_layer(row_top_frame, bn_20, GTextAlignmentCenter);
  text_time = create_text_layer(get_time_frame(), bn_69, GTextAlignmentCenter);
  row_bottom_frame = GRect(35, 111, 80, 27);
  text_row_bottom = create_text_layer(row_bottom_frame, bn_19, GTextAlignmentLeft);
  text_battery = create_text_layer(GRect(108, 111, 40, 21), bn_19, GTextAlignmentRight);
  text_temp = create_text_layer(GRect(48, 136, 41, 20), bn_19, GTextAlignmentRight);
#endif

  step_icon_top = layer_create(GRect(0, 0, bounds.size.w, row_top_frame.size.h));
  layer_set_update_proc(step_icon_top, step_icon_layer_update_proc);
  layer_set_hidden(step_icon_top, true);
  layer_add_child(window_layer, step_icon_top);

  step_icon_bottom = layer_create(GRect(0, 0, bounds.size.w, row_bottom_frame.size.h));
  layer_set_update_proc(step_icon_bottom, step_icon_layer_update_proc);
  layer_set_hidden(step_icon_bottom, true);
  layer_add_child(window_layer, step_icon_bottom);

  // getting battery info
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(500, 500);

  // to detect when timeline peek is shown
  unobstructed_area_service_subscribe((UnobstructedAreaHandlers){.change = unobstructed_changed, .did_change = unobstructed_did_change}, NULL);
  set_time_frame_for_unobstructed_area(layer_get_unobstructed_bounds(window_layer));

  // reading stored value
  if (persist_exists(KEY_WEATHER_CODE))
    show_icon(persist_read_int(KEY_WEATHER_CODE));
  if (persist_exists(KEY_WEATHER_TEMP))
    show_temperature(persist_read_int(KEY_WEATHER_TEMP));
  else
    text_layer_set_text(text_temp, "...");

  // initial bluetooth check
  flag_bluetooth_alert = 0;
  bluetooth_connection_service_subscribe(bluetooth_handler);
  bluetooth_handler(bluetooth_connection_service_peek());
  flag_bluetooth_alert = persist_exists(KEY_BLUETOOTH_ALERT) ? persist_read_int(KEY_BLUETOOTH_ALERT) : 1;

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

#if defined(PBL_HEALTH)
  // requires "health" in package.json capabilities
  health_service_events_subscribe(health_handler, NULL);
#endif

  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_did_change,
  });

  // Get a time structure so that the face doesn't start blank
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  // Manually call the tick handler when the window is loading
  tick_handler(t, DAY_UNIT | MINUTE_UNIT);
}

void handle_deinit(void)
{

  // clearning MASK
  text_layer_destroy(text_row_bottom);
  text_layer_destroy(text_time);
  text_layer_destroy(text_row_top);
  text_layer_destroy(text_battery);
  text_layer_destroy(text_temp);

  layer_destroy(step_icon_top);
  layer_destroy(step_icon_bottom);
  gbitmap_destroy(steps_icon_bitmap);
  gbitmap_destroy(meteoicons_all);
  gbitmap_destroy(meteoicon_current);
  bitmap_layer_destroy(temp_layer);

  layer_destroy(graphics_layer);

  window_destroy(my_window);
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  unobstructed_area_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  app_focus_service_unsubscribe();
}

int main(void)
{
  handle_init();
  app_event_loop();
  handle_deinit();
}
