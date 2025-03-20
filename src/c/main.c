#include <pebble.h>
#include "main.h"
#include <pebble-effect-layer/pebble-effect-layer.h>
#include "languages.h"

Window *my_window;
Layer *window_layer;

TextLayer *text_time, *text_date, *text_dow, *text_battery, *text_temp;
Layer *graphics_layer;
BitmapLayer *temp_layer;
GBitmap *meteoicons_all, *meteoicon_current;

GFont bn_69, bn_30, bn_26, bn_20, bn_19;

char s_date[] = "21  FEB  2015     "; // test
char s_time[] = "88.44mm";            // test
char s_dow[] = "WEDNESDAY     ";      // test
char s_battery[] = "100%";            // test
char s_temp[] = "-100°";

EffectLayer *effect_layer, *zoom_layer;

uint8_t flag_hoursMinutesSeparator, flag_dateFormat, flag_invertColors, flag_bluetooth_alert, flag_locationService, flag_weatherInterval, flag_language;
bool flag_messaging_is_busy = false, flag_js_is_ready = false;

GRect bounds;
GPoint center;

static void invert_colors()
{
  if (flag_invertColors == 1)
  {
    effect_layer_set_frame(effect_layer, bounds);
  }
  else
  {
    effect_layer_set_frame(effect_layer, GRectZero);
  }
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

static void toggle_weather_visibility()
{

  if (flag_locationService == 2)
  { // if weather disabled - hide it and move battery to center

    layer_set_hidden(text_layer_get_layer(text_temp), true);
    layer_set_hidden(bitmap_layer_get_layer(temp_layer), true);
#ifdef PBL_RECT
    layer_set_frame(text_layer_get_layer(text_battery), GRect(49, 0, 43, 21));
#endif
  }
  else
  { // otherwise show weather and move battery to edge

    layer_set_hidden(text_layer_get_layer(text_temp), false);
    layer_set_hidden(bitmap_layer_get_layer(temp_layer), false);
#ifdef PBL_RECT
    layer_set_frame(text_layer_get_layer(text_battery), GRect(98, 0, 43, 21));
#endif
  }
}

// calling for weather update
static void update_weather()
{
  // Only grab the weather if we can talk to phone AND weather is enabled AND currently message is not being processed and JS on phone is ready
  if (flag_locationService != 2 && bluetooth_connection_service_peek() && !flag_messaging_is_busy && flag_js_is_ready)
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
    int msg_result = app_message_outbox_send(); // need to assign result for successfull call
                                                // APP_LOG(APP_LOG_LEVEL_INFO, "**** I am inside 'update_weather()' message sent and result code = %d***", msg_result);
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

// handling time
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{

  char format[5];

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

    if (!(tick_time->tm_min % flag_weatherInterval) && (tick_time->tm_sec == 0))
    { // on configured weather interval change - update the weather
      // APP_LOG(APP_LOG_LEVEL_INFO, "**** I am inside 'tick_handler()' about to call 'update_weather();' at minute %d min on %d interval", tick_time->tm_min, flag_weatherInterval);
      update_weather();
    }
  }

  if (units_changed & DAY_UNIT)
  { // on day change - change date (format depends on flag)

    switch (flag_dateFormat)
    {
    case 0:
      if (flag_language == LANG_RUSSIAN || flag_language == LANG_POLISH)
      {                                                             // if this is Russian - need double bytes
        strftime(s_date, sizeof(s_date), "%b   -%d-%Y", tick_time); // "DEC 10 2015"
        strncpy(&s_date[0], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
      }
      else
      {
        strftime(s_date, sizeof(s_date), "%b-%d-%Y", tick_time); // "DEC 10 2015"
        if (flag_language != LANG_DEFAULT)
        { // if custom language is set - pull from language array
          strncpy(&s_date[0], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
        }
      }

      break;
    case 1:
      if (flag_language == LANG_RUSSIAN)
      {                                                             // if this is Russian - need double bytes
        strftime(s_date, sizeof(s_date), "%d-%b   -%Y", tick_time); // "DEC 10 2015"
        strncpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
      }
      else
      {
        strftime(s_date, sizeof(s_date), "%d-%b-%Y", tick_time); // "DEC 10 2015"
        if (flag_language != LANG_DEFAULT)
        { // if custom language is set - pull from language array
          strncpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
        }
      }

      break;
    case 2:
      strftime(s_date, sizeof(s_date), "%Y-%m-%d", tick_time); // "2015-12-10"
      break;
    }

    text_layer_set_text(text_date, s_date);

    if (flag_language != LANG_DEFAULT)
    { // if custom language is set - pull from language array
      text_layer_set_text(text_dow, LANG_DAY[flag_language][tick_time->tm_wday]);
    }
    else
    {
      strftime(s_dow, sizeof(s_dow), "%A", tick_time);
      text_layer_set_text(text_dow, s_dow);
    }
  }
}

void load_fonts()
{

  fonts_unload_custom_font(bn_69);
  fonts_unload_custom_font(bn_19);

  if (flag_language == LANG_RUSSIAN)
  {
    bn_69 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_69));
    bn_19 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_19));
  }
  else
  {
    bn_69 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_ENG_69));
    bn_19 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BIG_NOODLE_ENG_19));
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
    case KEY_INVERT_COLORS:
      if (t->value->int32 != flag_invertColors)
      {
        persist_write_int(KEY_INVERT_COLORS, t->value->int32);
        flag_invertColors = t->value->int32;
        invert_colors();
      }
      break;
    case KEY_LOCATION_SERVICE:
      if (t->value->int32 != flag_locationService)
      {
        persist_write_int(KEY_LOCATION_SERVICE, t->value->int32);
        flag_locationService = t->value->int32;
        toggle_weather_visibility();
        need_weather = 1;
        // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' location set to %d type", flag_locationService);
      }
      break;
    case KEY_WEATHER_INTERVAL:
      if (t->value->int32 != flag_weatherInterval && t->value->int32 != 1)
      { // precaution, dunno why we get 1 here as well
        persist_write_int(KEY_WEATHER_INTERVAL, t->value->int32);
        flag_weatherInterval = t->value->int32;
        need_weather = 1;
        // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' Weather interval set to interval to %d min", flag_weatherInterval);
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
  text_layer_set_text_color(text_layer, GColorWhite);
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

  graphics_context_set_antialiased(ctx, flag_invertColors == 0); // if we're doing inversion - disable antialiasing

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

// adjusting time location when timeline quickview shows. (original Y was hardcoded at 53 now doing percentage minus 15% of obstructed area)
void unobstructed_changed(GRect free_area, void *context)
{
  layer_set_frame(text_layer_get_layer(text_time), GRect(0, bounds.size.h * 53 / 168 - (bounds.size.h - free_area.size.h) * 15 / 100, bounds.size.w, 70));
}

void handle_init(void)
{

  //   // need to catch when app resumes focus after notification, otherwise effect layer won't restore
  //   app_focus_service_subscribe_handlers((AppFocusHandlers){
  //     .did_focus = app_focus_changed,
  //     .will_focus = app_focus_changing
  //   });

  // going international
  setlocale(LC_ALL, "");

  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_stack_push(my_window, true);

  window_layer = window_get_root_layer(my_window);
  bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&bounds);

  graphics_layer = layer_create(bounds);
  layer_set_update_proc(graphics_layer, graphics_update_proc);
  layer_add_child(window_layer, graphics_layer);

  meteoicons_all = gbitmap_create_with_resource(RESOURCE_ID_METEOICONS);
#ifdef PBL_RECT
  temp_layer = bitmap_layer_create(GRect(51, 1, 41, 20));
#else
  temp_layer = bitmap_layer_create(GRect(86, 137, 41, 21));
#endif
  bitmap_layer_set_compositing_mode(temp_layer, GCompOpSet);
  layer_add_child(graphics_layer, bitmap_layer_get_layer(temp_layer));

  flag_hoursMinutesSeparator = persist_exists(KEY_HOURS_MINUTES_SEPARATOR) ? persist_read_int(KEY_HOURS_MINUTES_SEPARATOR) : 0;
  flag_dateFormat = persist_exists(KEY_DATE_FORMAT) ? persist_read_int(KEY_DATE_FORMAT) : 0;
  flag_invertColors = persist_exists(KEY_INVERT_COLORS) ? persist_read_int(KEY_INVERT_COLORS) : 0;
  flag_bluetooth_alert = persist_exists(KEY_BLUETOOTH_ALERT) ? persist_read_int(KEY_BLUETOOTH_ALERT) : 0;
  flag_locationService = persist_exists(KEY_LOCATION_SERVICE) ? persist_read_int(KEY_LOCATION_SERVICE) : 0;
  flag_weatherInterval = persist_exists(KEY_WEATHER_INTERVAL) ? persist_read_int(KEY_WEATHER_INTERVAL) : 60; // default weather update is 1 hour
  flag_language = persist_exists(KEY_LANGUAGE) ? persist_read_int(KEY_LANGUAGE) : LANG_DEFAULT;              // default - language set by pebble

  load_fonts();

#ifdef PBL_RECT
  text_dow = create_text_layer(GRect(0, 30 * PBL_DISPLAY_HEIGHT / 168, bounds.size.w, 31 * PBL_DISPLAY_HEIGHT / 168), bn_30, GTextAlignmentCenter);
  text_time = create_text_layer(GRect(0, 53 * PBL_DISPLAY_HEIGHT / 168 + PBL_IF_HEIGHT_168_ELSE(0, 18), bounds.size.w, 70 * PBL_DISPLAY_HEIGHT / 168), bn_69, GTextAlignmentCenter);
  text_date = create_text_layer(GRect(0, 129 * PBL_DISPLAY_HEIGHT / 168, bounds.size.w, 27 * PBL_DISPLAY_HEIGHT / 168), bn_26, GTextAlignmentCenter);
  text_battery = create_text_layer(GRect(98, 0, 43, 21), bn_19, GTextAlignmentRight);
  text_temp = create_text_layer(GRect(3, 0, 80, 21), bn_19, GTextAlignmentLeft);
#else
  text_dow = create_text_layer(GRect(0, 29, bounds.size.w, 31), bn_20, GTextAlignmentCenter);
  text_time = create_text_layer(GRect(0, 38, bounds.size.w, 70), bn_69, GTextAlignmentCenter);
  text_date = create_text_layer(GRect(35, 111, 80, 27), bn_19, GTextAlignmentLeft);
  text_battery = create_text_layer(GRect(108, 111, 40, 21), bn_19, GTextAlignmentRight);
  text_temp = create_text_layer(GRect(48, 136, 41, 20), bn_19, GTextAlignmentRight);
#endif

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
  unobstructed_area_service_subscribe((UnobstructedAreaHandlers){.will_change = unobstructed_changed}, NULL);

  // reading stored value
  if (persist_exists(KEY_WEATHER_CODE))
    show_icon(persist_read_int(KEY_WEATHER_CODE));
  if (persist_exists(KEY_WEATHER_TEMP))
    show_temperature(persist_read_int(KEY_WEATHER_TEMP));
  else
    text_layer_set_text(text_temp, "...");

  effect_layer = effect_layer_create(bounds);
  effect_layer_add_effect(effect_layer, effect_invert_bw_only, NULL);
  layer_add_child(window_layer, effect_layer_get_layer(effect_layer));

  #ifdef PBL_PLATFORM_EMERY
    zoom_layer = effect_layer_create(GRect(0, 53 * PBL_DISPLAY_HEIGHT / 168 + 18, bounds.size.w, 70 * PBL_DISPLAY_HEIGHT / 168));
    effect_layer_add_effect(zoom_layer, effect_zoom, EL_ZOOM(139, 136)); 
    layer_add_child(window_layer, effect_layer_get_layer(zoom_layer));
  #endif

  invert_colors();             // initial check for inverting colors;
  toggle_weather_visibility(); // initial check for enable/disable weather

  // initial bluetooth check
  flag_bluetooth_alert = 0;
  bluetooth_connection_service_subscribe(bluetooth_handler);
  bluetooth_handler(bluetooth_connection_service_peek());
  flag_bluetooth_alert = persist_exists(KEY_BLUETOOTH_ALERT) ? persist_read_int(KEY_BLUETOOTH_ALERT) : 1;

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Get a time structure so that the face doesn't start blank
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  // Manually call the tick handler when the window is loading
  tick_handler(t, DAY_UNIT | MINUTE_UNIT);
}

void handle_deinit(void)
{

  // clearning MASK
  text_layer_destroy(text_date);
  text_layer_destroy(text_time);
  text_layer_destroy(text_dow);
  text_layer_destroy(text_battery);
  text_layer_destroy(text_temp);

  gbitmap_destroy(meteoicons_all);
  gbitmap_destroy(meteoicon_current);
  bitmap_layer_destroy(temp_layer);

  effect_layer_destroy(effect_layer);
  layer_destroy(graphics_layer);

  window_destroy(my_window);
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  unobstructed_area_service_unsubscribe();
  //   app_focus_service_unsubscribe();
}

int main(void)
{
  handle_init();
  app_event_loop();
  handle_deinit();
}
