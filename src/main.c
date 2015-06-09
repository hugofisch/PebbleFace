#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_accel_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
    /*if(direction > 0)
    {
      layer_set_hidden(text_layer_get_layer(s_time_layer),true);
      layer_set_hidden(text_layer_get_layer(s_accel_layer),false);
    }
    else
    {
      layer_set_hidden(text_layer_get_layer(s_time_layer),false);
      layer_set_hidden(text_layer_get_layer(s_accel_layer),true);
    }
    */
}

static void main_window_load(Window *window) {
  //---------------------------
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LIONS_LOGO2);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  //---------------------------
    
  //---------------------------
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 116, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_set_hidden(text_layer_get_layer(s_time_layer),false);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  //---------------------------
  
   
  //---------------------------
  //Setup Accel Layer
  s_accel_layer = text_layer_create(GRect(5, 116, 139, 50));
  text_layer_set_background_color(s_accel_layer, GColorClear);
  text_layer_set_text_color(s_accel_layer, GColorWhite);  
  text_layer_set_font(s_accel_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(s_accel_layer, "Accel");
  text_layer_set_text_alignment(s_accel_layer, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_accel_layer),true);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_accel_layer));
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
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
 
  //Subscribe to AccelerometerService
  accel_service_set_sampling_rate(100);
  accel_tap_service_subscribe(accel_tap_handler);

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