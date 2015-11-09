#include <pebble.h>

static Window *window;
static Layer *background_layer;
static Layer *numbers_layer;
static TextLayer *text_layer;

#define KEY_BLUE 10000

static const uint16_t numbers[10] = {
  0b111101101101111, //0
  0b110010010010111, //1
  0b111001111100111, //2
  0b111001111001111, //3
  0b101101111001001, //4
  0b111100111001111, //5
  0b111100111101111, //6
  0b111001001001001, //7
  0b111101111101111, //8
  0b111101111001111, //9
};

static uint8_t time_digits[4] = {0, 0, 0, 0};

static int8_t inset_active = 1;
static int8_t inset_inactive = -2;

static bool bluetooth_connection = true;

static void connection_handler(bool connected) {
  bluetooth_connection = connected;
  layer_mark_dirty(numbers_layer);
  vibes_short_pulse();
}

static void background_layer_redraw_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorWhite));
  for (uint8_t x = 0; x < 18; x++) {
   for (uint8_t y = 0; y < 21; y++) {
     graphics_fill_rect(ctx, GRect(x * 8 + inset_active, y * 8 + inset_active,
                  8 - 2 * inset_active, 8 - 2 * inset_active), 0, GCornerNone);
   }
  }
}

static void numbers_layer_redraw_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (uint8_t dig = 0; dig < 4; dig++) {
    if (dig < 10) {
      for (uint8_t x = 0; x < 3; x++) {
        for (uint8_t y = 0; y < 5; y++) {
          if (((numbers[time_digits[dig]] >> ((4 - y) * 3 + (2 - x))) & 1) == 1) {
            graphics_fill_rect(ctx, GRect((x + 4 * dig + 1 + (dig / 2)) * 8 + inset_inactive,
                    (y + 15) * 8 + inset_inactive, 8 - 2 * inset_inactive, 8 - 2 * inset_inactive), 0, GCornerNone);
          }
        }
      }
    }
  }
  if (!bluetooth_connection) {
    graphics_fill_rect(ctx, GRect(8 + inset_inactive, 8 + inset_inactive, 8 - 2 * inset_inactive, 8 - 2 * inset_inactive), 0, GCornerNone);
  }
  BatteryChargeState battery_state = battery_state_service_peek();
#if defined(PBL_COLOR)
  if (battery_state.is_charging) {
    graphics_context_set_fill_color(ctx, GColorGreen);
  } else if (battery_state.charge_percent <= 20) {
    graphics_context_set_fill_color(ctx, GColorRed);
  } else {
    graphics_context_set_fill_color(ctx, GColorLightGray);
  }
#elif defined(PBL_BW)
  graphics_context_set_fill_color(ctx, GColorWhite);
#endif
   graphics_fill_rect(ctx, GRect(16 * 8 + inset_active, 8 + inset_active,
                8 - 2 * inset_active, 8 - 2 * inset_active), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(16 * 8 + inset_inactive, 8 + inset_inactive, 8 - 2 * inset_inactive, (8 - 2 * inset_inactive) * (100 - battery_state.charge_percent) / 100), 0, GCornerNone);
}

static void update_time_digits(struct tm *tick_time, TimeUnits units_changed) {
  time_digits[0] = tick_time->tm_hour / 10;
  time_digits[1] = tick_time->tm_hour % 10;
  time_digits[2] = tick_time->tm_min / 10;
  time_digits[3] = tick_time->tm_min % 10;
  layer_mark_dirty(numbers_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  background_layer = layer_create(bounds);
  layer_set_update_proc(background_layer, background_layer_redraw_proc);
  layer_add_child(window_layer, background_layer);
  numbers_layer = layer_create(bounds);
  layer_set_update_proc(numbers_layer, numbers_layer_redraw_proc);
  layer_add_child(window_layer, numbers_layer);
}

static void window_unload(Window *window) {
  layer_destroy(background_layer);
  layer_destroy(numbers_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  time_t unix_time = time(NULL);
  struct tm *current_time = localtime(&unix_time);
  update_time_digits(current_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)update_time_digits);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = connection_handler
  });
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
