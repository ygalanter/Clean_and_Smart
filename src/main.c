#include <pebble.h>
#include "main.h"
#include "effect_layer.h"  
  
Window *my_window;  
Layer *window_layer;
  
TextLayer *text_time, *text_date, *text_dow, *text_battery, *text_temp;
Layer *graphics_layer;
BitmapLayer *temp_layer;
GBitmap *meteoicons_all, *meteoicon_current;

char s_date[] = "21  FEB  2015     "; //test
char s_time[] = "88.44mm"; //test
char s_dow[] = "WEDNESDAY     "; //test  
char s_battery[] = "100%"; //test
char s_temp[] = "-100°";

EffectLayer *effect_layer;

uint8_t flag_hoursMinutesSeparator, flag_dateFormat, flag_invertColors, flag_bluetoothBuzz, flag_locationService, flag_weatherInterval;
bool flag_messaging_is_busy = false;


static void toggle_weather_visibility() {
  
  if (flag_locationService == 2) { // if weather disabled - hide it and move battery to center
    
    layer_set_hidden(text_layer_get_layer(text_temp), true);
    layer_set_hidden(bitmap_layer_get_layer(temp_layer), true);
    layer_set_frame(text_layer_get_layer(text_battery), GRect(49, -1, 43, 21));
    
  } else { // otherwise show weather and move battery to edge
    
    layer_set_hidden(text_layer_get_layer(text_temp), false);
    layer_set_hidden(bitmap_layer_get_layer(temp_layer), false);
    layer_set_frame(text_layer_get_layer(text_battery), GRect(98, -1, 43, 21));
   
  }
  
  
}


static void invert_colors() {
  if (flag_invertColors == 1) {
     effect_layer = effect_layer_create(GRect(0,0,144,168));
     effect_layer_add_effect(effect_layer, effect_invert_bw_only, NULL);
     layer_add_child(window_layer, effect_layer_get_layer(effect_layer));
  } else {
     effect_layer_destroy(effect_layer);
  }
  
}

//calling for weather update
static void update_weather() {
  // Only grab the weather if we can talk to phone AND weather is enabled AND currently message is not being processed
  if (flag_locationService != 2 && bluetooth_connection_service_peek() && !flag_messaging_is_busy) {
    // APP_LOG(// APP_LOG_LEVEL_INFO, "**** I am inside 'update_weather()' about to request weather from the phone ***");
    
//     DictionaryIterator *iter;
//     app_message_outbox_begin(&iter);
//     Tuplet dictionary[] = {
//       MyTupletCString(KEY_UNITS, w_units),
//       TupletInteger(KEY_TIMESWITCH, time_switch),
//     };
//     dict_write_tuplet(iter, &dictionary[0]);
//     dict_write_tuplet(iter, &dictionary[1]);
     flag_messaging_is_busy = true;
     int msg_result = app_message_outbox_send();
    // APP_LOG(// APP_LOG_LEVEL_INFO, "**** I am inside 'update_weather()' message sent and result code = %d***", msg_result);
  } 
}

// showing temp
static void show_temperature(int w_current) {
    // APP_LOG(// APP_LOG_LEVEL_INFO, "**** I am inside 'show_temperature()'; TEMP in Pebble: %d", w_current);
    static char buffer[6];
    snprintf(buffer, sizeof(buffer), "%i\u00B0", w_current);
    text_layer_set_text(text_temp, buffer);
}

//showing weather icon
static void show_icon(int w_icon) {
   if (meteoicon_current)  gbitmap_destroy(meteoicon_current);
   meteoicon_current =  gbitmap_create_as_sub_bitmap(meteoicons_all, GRect(0, 20*w_icon, 41, 20));
   bitmap_layer_set_bitmap(temp_layer, meteoicon_current);
}

//handling time
void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
   char format[5];
     
   // building format 12h/24h
   if (clock_is_24h_style()) {
      strcpy(format, "%H:%M"); // e.g "14:46"
   } else {
      strcpy(format, "%l:%M"); // e.g " 2:46" -- with leading space
   }
  
  
   // if separator is dot = replacing colon with it
   if (flag_hoursMinutesSeparator == 1) format[2] = '.';
  
   if (units_changed & MINUTE_UNIT) { // on minutes change - change time
     strftime(s_time, sizeof(s_time), format, tick_time);
     
     if (s_time[0] == ' ') { // if in 12h mode we have leading space in time - don't display it (it will screw centering of text) start with next char
       text_layer_set_text(text_time, &s_time[1]);
     } else {
       text_layer_set_text(text_time, s_time);  
     }
     
     if (!(tick_time->tm_min % flag_weatherInterval)) { // on configured weather interval change - update the weather
        // APP_LOG(// APP_LOG_LEVEL_INFO, "**** I am inside 'tick_handler()' about to call 'update_weather();' at minute %d min on %d interval", tick_time->tm_min, flag_weatherInterval);
        update_weather();
     } 
   }  

  
   
   if (units_changed & DAY_UNIT) { // on day change - change date (format depends on flag)
     
     switch(flag_dateFormat){
       case 0:
         strftime(s_date, sizeof(s_date), "%b  %d  %Y", tick_time); // "DEC 10 2015"
         break;
       case 1:
         strftime(s_date, sizeof(s_date), "%d  %b  %Y", tick_time); // "10 DEC 2015"
         break;
       case 2:
         strftime(s_date, sizeof(s_date), "%Y-%m-%d", tick_time);  // "2015-12-10"
         break;
       
     }

     text_layer_set_text(text_date, s_date);
   
     strftime(s_dow, sizeof(s_dow), "%A", tick_time);
     text_layer_set_text(text_dow, s_dow);
   }
  
}



static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' Message from the phone received!");

  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  bool need_weather = 0;
  bool need_time = 0;

    // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      
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
        if (t->value->int16) {
          // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' message 'JS is ready' received !");
          need_weather = 1;
        }
        break;
      
       // config keys
      case KEY_TEMPERATURE_FORMAT: //if temp format changed from F to C or back - need re-request weather
        // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' switching temp format");
        need_weather = 1;
      case KEY_HOURS_MINUTES_SEPARATOR:
        if (t->value->int32 != flag_hoursMinutesSeparator) {
          persist_write_int(KEY_HOURS_MINUTES_SEPARATOR, t->value->int32);
          flag_hoursMinutesSeparator = t->value->int32;
          need_time = 1;
        } 
        break;
      case KEY_DATE_FORMAT:
        if (t->value->int32 !=flag_dateFormat) {
          persist_write_int(KEY_DATE_FORMAT, t->value->int32);
          flag_dateFormat = t->value->int32;
          need_time = 1;
        }  
        break;
      case KEY_BLUETOOTH_BUZZ:
        if (t->value->int32 !=flag_bluetoothBuzz) {
          persist_write_int(KEY_BLUETOOTH_BUZZ, t->value->int32);
          flag_bluetoothBuzz = t->value->int32;
        }  
        break;
      case KEY_INVERT_COLORS:
        if (t->value->int32 != flag_invertColors) {
          persist_write_int(KEY_INVERT_COLORS, t->value->int32);
          flag_invertColors = t->value->int32;
          invert_colors();
        }
        break;
      case KEY_LOCATION_SERVICE:
         if (t->value->int32 != flag_locationService) {
           persist_write_int(KEY_LOCATION_SERVICE, t->value->int32);
           flag_locationService = t->value->int32;
           toggle_weather_visibility();  
           need_weather = 1;
           // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' location set to %d type", flag_locationService);
         }  
      case KEY_WEATHER_INTERVAL:
      if (t->value->int32 != flag_weatherInterval && t->value->int32 !=1) { // precaution, dunno why we get 1 here as well
           persist_write_int(KEY_WEATHER_INTERVAL, t->value->int32);
           flag_weatherInterval = t->value->int32;
           need_weather = 1;
           // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' Weather interval set to interval to %d min", flag_weatherInterval);
         }  
      
      }   
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (need_weather) {
    // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' about to call 'update_weather();");
    update_weather();
  }
  
  if (need_time) {
      //Get a time structure 
      time_t temp = time(NULL);
      struct tm *t = localtime(&temp);
 
      //Manually call the tick handler
      tick_handler(t, MINUTE_UNIT | DAY_UNIT);
  }
  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  // APP_LOG(// APP_LOG_LEVEL_ERROR, "____Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  flag_messaging_is_busy = false;
  // APP_LOG(// APP_LOG_LEVEL_ERROR, "____Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  flag_messaging_is_busy = false;
  // APP_LOG(// APP_LOG_LEVEL_INFO, "_____Outbox send success!");
} 
  

//creates text layer at given coordinates, given font and alignment  
TextLayer* create_text_layer(GRect coords, int font, GTextAlignment align) {
  TextLayer *text_layer = text_layer_create(coords);
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(font)));
  text_layer_set_text_color(text_layer, GColorWhite);  
  text_layer_set_background_color(text_layer, GColorClear);  
  text_layer_set_text_alignment(text_layer, align);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  return text_layer;   
}  


static void bluetooth_handler(bool state) {
  
  if (flag_bluetoothBuzz == 1) vibes_short_pulse();
  
  if (state){
    // APP_LOG(// APP_LOG_LEVEL_INFO, "***** I am inside of 'bluetooth_handler()' about to call 'update_weather();");
    update_weather();
  } 
    
  layer_mark_dirty(graphics_layer);
  
}


static void graphics_update_proc(Layer *layer, GContext *ctx) {

  static GColor color;  
  
  #ifdef PBL_COLOR
    
// doing battery color in ranges with fall thru:
//       100% - 50% - GColorJaegerGreen
//       49% - 20% - GColorChromeYellow
//       19% - 0% - GColorDarkCandyAppleRed

    
     switch (battery_state_service_peek().charge_percent) {
       case 100: 
       case 90: 
       case 80: 
       case 70: 
       case 60: 
       case 50: color = GColorJaegerGreen; break;
       case 40: 
       case 30: 
       case 20: color = GColorChromeYellow; break;
       case 10: 
       case 0:  color = GColorDarkCandyAppleRed; break;     
   }
   #else
     color = GColorWhite;
   #endif
   
   graphics_context_set_fill_color(ctx, color);
   graphics_fill_rect(ctx, GRect(0,25,144,3), 0, GCornersAll);
  
  if (bluetooth_connection_service_peek()) {
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, GColorCyan);
    #else
      graphics_context_set_fill_color(ctx, GColorWhite);
    #endif
      
    graphics_fill_rect(ctx, GRect(0,164,144,3), 0, GCornersAll);  
  }
  
}  
  


static void battery_handler(BatteryChargeState state) {
  snprintf(s_battery, sizeof("100%"), "%d%%", state.charge_percent);
  text_layer_set_text(text_battery, s_battery);
}



void handle_init(void) {
  
  //going international
  setlocale(LC_ALL, "");
  
  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_stack_push(my_window, true);
  
  #ifdef PBL_PLATFORM_APLITE
    window_set_fullscreen(my_window, true);
  #endif  
    
  window_layer = window_get_root_layer(my_window);  
  
  graphics_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(graphics_layer, graphics_update_proc);
  layer_add_child(window_layer, graphics_layer);
  
  text_dow = create_text_layer(GRect(0,29,144,31), RESOURCE_ID_BIG_NOODLE_30, GTextAlignmentCenter);
  text_time = create_text_layer(GRect(-20,52,184,70), RESOURCE_ID_BIG_NOODLE_69, GTextAlignmentCenter);
  text_date = create_text_layer(GRect(0,128,144,27), RESOURCE_ID_BIG_NOODLE_26, GTextAlignmentCenter);
  text_battery = create_text_layer(GRect(98, -1, 43, 21), RESOURCE_ID_BIG_NOODLE_20, GTextAlignmentRight);
  text_temp = create_text_layer(GRect(3, -1, 80, 21), RESOURCE_ID_BIG_NOODLE_20, GTextAlignmentLeft);
 
  //getting battery info
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
  
  meteoicons_all = gbitmap_create_with_resource(RESOURCE_ID_METEOICONS);
  temp_layer =  bitmap_layer_create(GRect(51, 1, 41, 20));
  layer_add_child(window_layer, bitmap_layer_get_layer(temp_layer));
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum()); 
  
  
   // reading stored value
  if (persist_exists(KEY_WEATHER_CODE)) show_icon(persist_read_int(KEY_WEATHER_CODE));
  if (persist_exists(KEY_WEATHER_TEMP))
    show_temperature(persist_read_int(KEY_WEATHER_TEMP));
  else
    text_layer_set_text(text_temp, "...");
   
  
  flag_hoursMinutesSeparator = persist_exists(KEY_HOURS_MINUTES_SEPARATOR)? persist_read_int(KEY_HOURS_MINUTES_SEPARATOR) : 0;
  flag_dateFormat = persist_exists(KEY_DATE_FORMAT)? persist_read_int(KEY_DATE_FORMAT) : 0;
  flag_invertColors = persist_exists(KEY_INVERT_COLORS)? persist_read_int(KEY_INVERT_COLORS) : 0;
  flag_bluetoothBuzz = persist_exists(KEY_BLUETOOTH_BUZZ)? persist_read_int(KEY_BLUETOOTH_BUZZ) : 0;
  flag_locationService = persist_exists(KEY_LOCATION_SERVICE)? persist_read_int(KEY_LOCATION_SERVICE) : 0;
  flag_weatherInterval = persist_exists(KEY_WEATHER_INTERVAL)? persist_read_int(KEY_WEATHER_INTERVAL) : 60; // default weather update is 1 hour
  
  
  invert_colors(); //initial check for inverting colors;
  toggle_weather_visibility(); //initial check for enable/disable weather
  
  // initial bluetooth check
  flag_bluetoothBuzz = 0;
  bluetooth_connection_service_subscribe(bluetooth_handler);
  bluetooth_handler(bluetooth_connection_service_peek());
  flag_bluetoothBuzz = persist_exists(KEY_BLUETOOTH_BUZZ)? persist_read_int(KEY_BLUETOOTH_BUZZ) : 0;
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
   //Get a time structure so that the face doesn't start blank
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
 
  //Manually call the tick handler when the window is loading
  tick_handler(t, DAY_UNIT | MINUTE_UNIT);
  
  
 
  
}

void handle_deinit(void) {
  
  //clearning MASK
  text_layer_destroy(text_date);
  text_layer_destroy(text_time);
  text_layer_destroy(text_dow);
  text_layer_destroy(text_battery);
  
  gbitmap_destroy(meteoicons_all);
  gbitmap_destroy(meteoicon_current);
  bitmap_layer_destroy(temp_layer);
  
  effect_layer_destroy(effect_layer);
  
  window_destroy(my_window);
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  
}
  

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}