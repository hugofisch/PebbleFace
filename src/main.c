#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_battery_layer;
static GBitmap *s_battery_bitmap;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bitmap;
static TextLayer *s_countdown_layer;
static TextLayer *s_vs_layer;

static uint8_t s_charge_percent=0;
static bool s_is_charging=false;
static bool s_is_plugged=false;
static bool s_connected=false;
static time_t s_displaytimer=0;
static bool s_displaycountdown=false;

#define NUM_GAMEDAYS 7
typedef struct {
 char *name;
 time_t time;
  bool home;
} GameDay;


// Time calculated from https://www.aritso.net/mehr-informationen/tools/timestampconverter.htm
// locatime UTC+2 !!!
static GameDay s_game_day[] = {
 { .name = "vs Unicorns", .time = 1434819600, .home = true },
 { .name = "vs Huskies", .time = 1437764400, .home = true },
 { .name = "Hurricanes vs", .time = 1438441200, .home = false },
{ .name = "vs Rebbels", .time = 1439046000, .home = true },
  { .name = "Huskies vs", .time = 1439654400, .home = false },
  { .name = "Panther vs", .time = 1440259200, .home = false },
  { .name = "vs Adler", .time = 1441472400, .home = true }
};


static void show_time_or_countdown()
{
  layer_set_hidden(text_layer_get_layer(s_time_layer),s_displaycountdown);
  layer_set_hidden(text_layer_get_layer(s_date_layer),s_displaycountdown);
  layer_set_hidden(text_layer_get_layer(s_countdown_layer),!s_displaycountdown);
  layer_set_hidden(text_layer_get_layer(s_vs_layer),!s_displaycountdown);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  static char datebuffer[] = "00. Mon";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  
  // Date
  strftime(datebuffer, sizeof("00. Mon"), "%d. %b", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_date_layer, datebuffer);
  
  if (s_is_charging || s_is_plugged)
  {
    gbitmap_destroy(s_battery_bitmap);
    s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
  } else {
    if (s_charge_percent<21)
    {
      gbitmap_destroy(s_battery_bitmap);
      s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_EMPTY);
      bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap); 
    }
    if ((s_charge_percent>=21)&&(s_charge_percent<50))
    {
      gbitmap_destroy(s_battery_bitmap);
      s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_25);
      bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap); 
    }
    if ((s_charge_percent>=50)&&(s_charge_percent<75))
    {
      gbitmap_destroy(s_battery_bitmap);
      s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_50);
      bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap); 
    }
    if ((s_charge_percent>=75)&&(s_charge_percent<=90))
    {
      gbitmap_destroy(s_battery_bitmap);
      s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_75);
      bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap); 
    }
    if ((s_charge_percent>90))
    {
      gbitmap_destroy(s_battery_bitmap);
      s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_FULL);
      bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap); 
    }    
    if (s_connected)
    {
      gbitmap_destroy(s_bluetooth_bitmap);
      s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_WHITE_OK);
      bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap); 
    }
    else
    {
      gbitmap_destroy(s_bluetooth_bitmap);
      s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_WHITE_NOTOK);
      bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);       
    }
  }
  
  if (s_displaycountdown)
  {
    show_time_or_countdown();
    time_t temptime = time(NULL); 
    int timetodisplay=(int)(temptime-s_displaytimer);
    if (timetodisplay<3){
      static char diffbuffer[] = "000:00:00:00";
      bool found=false;
      for (int i=0;i<NUM_GAMEDAYS;i++){
        GameDay *gameday = &s_game_day[i];
        time_t diff=difftime(gameday->time,temptime);
        if (diff>=0){
          int nDays,nHours,nMinutes,nSeconds;
          nDays=(int)diff/(60*60*24);
          nHours=(int)(diff-60*60*24*nDays)/(60*60);
          nMinutes=(int)(diff-60*60*24*nDays-60*60*nHours)/60;
          nSeconds=(int)(diff-60*60*24*nDays-60*60*nHours-60*nMinutes);
          snprintf(diffbuffer, sizeof("000:00:00:00"),"%03d:%02d:%02d:%02d",nDays,nHours,nMinutes,nSeconds);
          text_layer_set_text(s_countdown_layer, diffbuffer);
          text_layer_set_text(s_vs_layer, gameday->name);
          found=true;
          break;
        }
      }  
      if (!found){
        text_layer_set_text(s_countdown_layer, "000:00:00:00");
        text_layer_set_text(s_vs_layer, "unknown");
      }
    } else {
      s_displaycountdown=false;
      show_time_or_countdown();
    }
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
    if (!s_displaycountdown){
      s_displaytimer=time(NULL);
      s_displaycountdown=true;
    }
}

static void batterie_handler(BatteryChargeState charge)
{
  s_charge_percent=charge.charge_percent;
  s_is_charging=charge.is_charging;
  s_is_plugged=charge.is_plugged;
}

static void bluetooth_handler(bool connected)
{
  s_connected=connected;
}

static void main_window_load(Window *window) {
  //---------------------------
  // Create Background Logo GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIONS_LOGO2);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  //---------------------------

  
  //---------------------------
  // Create Battery GBitmap, then set to created BitmapLayer
  s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_EMPTY);
  s_battery_layer = bitmap_layer_create(GRect(118, 0, 25, 15));
  bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_layer));
  //---------------------------
  
  
  //---------------------------
  // Create Bluetooth GBitmap, then set to created BitmapLayer
  s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_WHITE_NOTOK);
  s_bluetooth_layer = bitmap_layer_create(GRect(0, 0, 20, 20));
  bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
  //---------------------------
  
  //---------------------------
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 118, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_set_hidden(text_layer_get_layer(s_time_layer),false);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  //---------------------------
  
  
  //---------------------------
    // Create date TextLayer
  s_date_layer = text_layer_create(GRect(5, 148, 139, 18));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "00. Jan");
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  // Add it as a child layer to the Window's root layer
  layer_set_hidden(text_layer_get_layer(s_date_layer),false);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  //---------------------------
   
  //---------------------------
  //Setup Countdown Layer
  s_countdown_layer = text_layer_create(GRect(5, 122, 139, 18));
  text_layer_set_background_color(s_countdown_layer, GColorClear);
  text_layer_set_text_color(s_countdown_layer, GColorWhite);  
  text_layer_set_font(s_countdown_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_countdown_layer, "000:00:00:00");
  text_layer_set_text_alignment(s_countdown_layer, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_countdown_layer),true);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_countdown_layer));
  //---------------------------

  //---------------------------
  //Setup VS Layer
  s_vs_layer = text_layer_create(GRect(5, 140, 139, 18));
  text_layer_set_background_color(s_vs_layer, GColorClear);
  text_layer_set_text_color(s_vs_layer, GColorWhite);  
  text_layer_set_font(s_vs_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_vs_layer, "vs unkown");
  text_layer_set_text_alignment(s_vs_layer, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_vs_layer),true);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_vs_layer));
  //---------------------------

  
}

static void main_window_unload(Window *window) {
     
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
  
    // Destroy GBitmap
    gbitmap_destroy(s_background_bitmap);
    
    // Destroy BitmapLayer
    bitmap_layer_destroy(s_background_layer);
  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
 
  //Subscribe to AccelerometerService
  accel_service_set_sampling_rate(100);
  accel_tap_service_subscribe(accel_tap_handler);

  //Subscribe BatterieService
  battery_state_service_subscribe(batterie_handler);
   batterie_handler(battery_state_service_peek());
  
  //Subscribe BluetoothService
  bluetooth_connection_service_subscribe(bluetooth_handler);
  bluetooth_handler(bluetooth_connection_service_peek());
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}