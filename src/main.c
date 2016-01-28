#include <pebble.h>
#include "OutlinedTextLayer.h"
#include "characters.h"

#define YEAR_MONKEY 1454860800

static Window *main_window;
static OutlinedTextLayer *time_layer;
static GBitmap *animal;
static BitmapLayer *animal_layer;
static GBitmap *fu;
static BitmapLayer *fu_layer;
static OutlinedTextLayer *date_layer;
static OutlinedTextLayer *week_layer;
static GFont font_chinese;
static GFont font_numbers;

static GRect next_animal_position(GRect bounds) {
    int r = rand() % PBL_IF_ROUND_ELSE(2, 3);
    GSize animal_size = gbitmap_get_bounds(animal).size;
    GPoint origin;
    switch(r) {
        case 0:
            origin = (GPoint){1, bounds.size.h - animal_size.h - 1};
            break;
        case 1:
#ifndef PBL_ROUND
            origin = (GPoint){74 - animal_size.w/2, 29};
            break;
        case 2:
#endif
            origin = (GPoint){bounds.size.w - animal_size.w - 1, bounds.size.h - animal_size.h - 1};
            break;
    };
    return (GRect) {
        .origin=origin,
        .size=animal_size
    };
}

//Choose a position for your GOAT or MONKEY
static void reposition_animal() {
    layer_set_frame(bitmap_layer_get_layer(animal_layer), next_animal_position(layer_get_bounds(window_get_root_layer(main_window))));
}

bool show_lantern = true;
static const GPathInfo RED_SQUARE = {
        4,
        (GPoint[]) {{-71,  0},{0, 71}, {71, 0}, {0, -71}}
};
static GPath *redsq_path;

//STRANDS
// I super over engineered this and it still looks like spaghetti gone wrong
// Started out as a random walk but then I modified it and modified it again
static Layer *redsq_layer;
static Layer *lantern_layer;
static Layer *strand_layer;
static int8_t lantern_inner_width;
static uint8_t inner_offset_x;
static AnimationProgress strand_anim_progress;
#define STRAND_LEN 32
static int32_t strand_directions[STRAND_LEN];
uint8_t real_strand_len = STRAND_LEN;
#define DOWN 0
#define LEFT 1
static uint8_t bias = LEFT;
#define RIGHT 2
//Stays same as last direction
#define N_PERSISTENCE 2
#define N_GRAVITY 2
#define N_BIAS 2
#define N_DIRS (N_PERSISTENCE + N_BIAS + N_GRAVITY + 3)
//Strand obeys strand_direction
#define P_CONFORMS 5
//This position on the strand_direction changes on each time step
#define STRAND_SEP 5
#define LANTERN_MARGIN_X PBL_IF_ROUND_ELSE(25, 10)
#define LANTERN_MARGIN_Y PBL_IF_ROUND_ELSE(34, 34)

static void draw_strands(Layer* layer, GContext * ctx) {
    //Strands
    graphics_context_set_stroke_color(ctx, GColorYellow);
    int direction_last = DOWN;
    uint8_t n_strands = (lantern_inner_width/STRAND_SEP)+1;
    for(int s=0; s< n_strands; s++) {
        GPoint strand = (GPoint){inner_offset_x + s*STRAND_SEP, 0};
        graphics_draw_pixel(ctx, strand);
        graphics_draw_pixel(ctx, (GPoint){strand.x+1, strand.y});
        for(int i=0;i<real_strand_len;i++) {
            int direction = strand_directions[i];
            if(rand() % P_CONFORMS == 0) // usually follow the other strands, but sometimes don't conform
                direction = rand() % (N_DIRS);
            
            if(direction > RIGHT + N_GRAVITY + N_BIAS) {
                //Persistence
                direction = direction_last;
            } else if(direction > RIGHT + N_GRAVITY + N_BIAS) {
                //Wind bias direction
                direction = bias;
            } else if(direction > RIGHT) {
                direction = DOWN;
            }
            
            if(direction == DOWN) strand.y++;
            else if(direction == LEFT) strand.x--;
            else if(direction == RIGHT) strand.x++;
                
            graphics_draw_pixel(ctx, strand);
            graphics_draw_pixel(ctx, (GPoint){strand.x+1, strand.y});
            graphics_draw_pixel(ctx, (GPoint){strand.x, strand.y+1});
            direction_last = direction;
        }
    }
}

static AnimationProgress anim_progress;
static void animation_started(Animation *anim, void *context) {
    strand_anim_progress = 0;
    real_strand_len = STRAND_LEN;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
#ifdef PBL_PLATFORM_APLITE
  animation_destroy(anim);
#endif
}

static void animate(int duration, int delay, AnimationImplementation *implementation, bool handlers) {
  Animation *anim = animation_create();
  if(anim) {
    animation_set_duration(anim, duration);
    animation_set_delay(anim, delay);
    animation_set_curve(anim, AnimationCurveEaseInOut);
    animation_set_implementation(anim, implementation);
    if(handlers) {
      animation_set_handlers(anim, (AnimationHandlers) {
        .started = animation_started,
        .stopped = animation_stopped
      }, NULL);
    }
    animation_schedule(anim);
  }
}

static void anim_upd(Animation *anim, AnimationProgress dist_normalized) {
    int dt = dist_normalized - strand_anim_progress;
    if(dt > 500) {
        for(int d=0;d<STRAND_LEN;d++) {
            if(rand() % 5 == 0)
                strand_directions[d] = rand() % N_DIRS;
        }
        anim_progress = dist_normalized;
    }
    if(dist_normalized < ANIMATION_NORMALIZED_MAX / 3) bias = LEFT;
    else if(dist_normalized < ANIMATION_NORMALIZED_MAX * 2 / 3) bias = RIGHT;
    else bias = DOWN;
        
    if(dist_normalized > ANIMATION_NORMALIZED_MAX / 2)
        real_strand_len = STRAND_LEN * ANIMATION_NORMALIZED_MAX / dist_normalized / 2;
    layer_mark_dirty(lantern_layer);
}

static AnimationImplementation anim_impl = {
    .update = anim_upd
  };

// END STRANDS

// Swap lantern and red square
static void toggle_lantern() {
    show_lantern = !layer_get_hidden(redsq_layer);
    layer_set_hidden(redsq_layer, show_lantern);
    layer_set_hidden(lantern_layer, !show_lantern);
    layer_set_hidden(strand_layer, !show_lantern);
    if(show_lantern) {
        animate(7*1000, 0, &anim_impl, true);
    }
}

static void draw_redsq(Layer* layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, GColorRed);
    gpath_draw_filled(ctx, redsq_path);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 2);
    gpath_draw_outline(ctx, redsq_path);
}

static void draw_lantern(Layer* layer, GContext * ctx) {
    if(!show_lantern) return;    
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    GPoint origin = (GPoint){0,6};
    uint8_t radius = (bounds.size.w) / 2;
    uint8_t dAngle = 40;
    int32_t height = sin_lookup(DEG_TO_TRIGANGLE(dAngle)) * 2 * radius / TRIG_MAX_RATIO;
    lantern_inner_width = cos_lookup(DEG_TO_TRIGANGLE(dAngle)) * 2 * radius / TRIG_MAX_RATIO;
    inner_offset_x = (radius * 2 - lantern_inner_width) / 2;
    //sin(dAngle) = (height/2) / radius
    //(Draw a triangle)
    GRect rounded = (GRect){
        .origin= origin,
        .size={
            radius * 2,
            height
        }
    };
    GRect mid = (GRect){
        .origin= { origin.x + inner_offset_x, origin.y },
        .size={
            lantern_inner_width,
            height
        }
    };
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, mid, 0, GCornerNone);
    graphics_fill_radial(ctx, rounded, GOvalScaleModeFillCircle, 1000, DEG_TO_TRIGANGLE(90-dAngle), DEG_TO_TRIGANGLE(90+dAngle));
    graphics_fill_radial(ctx, rounded, GOvalScaleModeFillCircle, 1000, DEG_TO_TRIGANGLE(270-dAngle), DEG_TO_TRIGANGLE(270+dAngle));
    GRect hat = (GRect){
        .origin={mid.origin.x, origin.y-6},
        .size={lantern_inner_width, 6}
    };
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_rect(ctx, hat, 0, GCornerNone);
    
    layer_set_frame(strand_layer, (GRect) {
        .origin={LANTERN_MARGIN_X, LANTERN_MARGIN_Y+ origin.y + height},
        .size={bounds.size.w, STRAND_LEN}
    });
}

static void hide_fu_show_clock(void * ignored) {
    layer_set_hidden(bitmap_layer_get_layer(fu_layer), true);
    layer_set_hidden(outlined_text_layer_get_layer(time_layer), false);
}

static void hide_clock_show_fu(void * ignored) {
    layer_set_hidden(bitmap_layer_get_layer(fu_layer), false);
    layer_set_hidden(outlined_text_layer_get_layer(time_layer), true);
    app_timer_register(1000, hide_fu_show_clock, NULL);
}

//STRANDS

static void tick_minute_handler(struct tm *tick_time, TimeUnits units_changed) {
    /*/<!--Screenshots
    tick_time->tm_min = 45;
    tick_time->tm_hour = 12;
    tick_time->tm_mon = 5;
    tick_time->tm_wday = 0;
    app_timer_register(2000, hide_clock_show_fu, NULL);
    toggle_lantern();
    //-->*/
    hide_fu_show_clock(NULL);
    static char time_buffer[8];
	// Write the current hours and minutes into a buffer
    if(tick_time->tm_min % 30 == 0) {
        outlined_text_layer_set_font(time_layer, font_chinese);
        chinese_time_string(time_buffer, tick_time);
    } else {
        outlined_text_layer_set_font(time_layer, font_numbers);
        strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ?
                                               "%H:%M" : "%l:%M", tick_time);
    }
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Time is: %s", time_buffer);
	outlined_text_layer_set_text(time_layer, time_buffer);
    
    static char date_buffer[9];    
    static char week_buffer[4];
    if(!units_changed || (units_changed & DAY_UNIT)) {
        chinese_date_string(date_buffer, tick_time);
        outlined_text_layer_set_text(date_layer, date_buffer);
        week_buffer[0] = XING;
        week_buffer[1] = QI;
        week_buffer[2] = chinese_weekday_number(tick_time);
        week_buffer[3] = '\0';
        outlined_text_layer_set_text(week_layer, week_buffer);
    }

    if((units_changed & HOUR_UNIT) && show_lantern) {
        animate(4*1000, 0, &anim_impl, true);
    }
    
    if(!units_changed) {
        // Kick off
        // We could keep checking for monkey time but clockface will restart whenever a notification etc happens
        time_t nowUTC = mktime(tick_time);
        animal = gbitmap_create_with_resource(nowUTC < YEAR_MONKEY ? RESOURCE_ID_GOAT : RESOURCE_ID_MONKEY);
        bitmap_layer_set_bitmap(animal_layer, animal);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "~%d days until monkey time :)", (int)((YEAR_MONKEY-nowUTC)/60/60/24+1));
    }
    
    reposition_animal();
}

AppTimer* time_until_can_tap_again = NULL;
static void reallow_taps(void * data) {
    time_until_can_tap_again = NULL;
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
    if(time_until_can_tap_again != NULL) {
        app_timer_reschedule(time_until_can_tap_again, 10000);
    } else {
        time_until_can_tap_again = app_timer_register(10000, reallow_taps, NULL);
        reposition_animal();
        toggle_lantern();
        // Show clock for 2 seconds
        // Then briefly show the fu
        app_timer_register(2000, hide_clock_show_fu, NULL);
    }
}

static bool bluetooth = false;
static void battery_callback(BatteryChargeState state) {
    if(bluetooth) {
        if(state.is_charging || state.is_plugged || state.charge_percent > 25) {
            window_set_background_color(main_window, GColorElectricUltramarine);
        } else {
            window_set_background_color(main_window, GColorRajah);
        }
    }
}
static void bluetooth_callback(bool connected) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, connected ? "bluetooth" : "no bluetooth");
    if(bluetooth && !connected) {
        // Notify that phone has disconnected
        vibes_short_pulse();
        window_set_background_color(main_window, GColorBlack);
    } else if(!bluetooth && connected) {
        window_set_background_color(main_window, GColorElectricUltramarine);
        battery_callback(battery_state_service_peek());
    }
    bluetooth = connected;
}

static void main_window_load(Window *window) {
    connection_service_subscribe((ConnectionHandlers) {
      .pebble_app_connection_handler = bluetooth_callback
    });
    bluetooth_callback(connection_service_peek_pebble_app_connection());
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());
    
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
        
    redsq_path = gpath_create(&RED_SQUARE);
    gpath_move_to(redsq_path, (GPoint){bounds.size.w/2, bounds.size.h/2});
    font_chinese = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_WOZIKU_28));
    font_numbers = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SHANGHAI_40));

	//Make layer
	time_layer = outlined_text_layer_create(GRect(5, bounds.size.h/2-29, bounds.size.w - 10, 45));
	outlined_text_layer_set_colors(time_layer, GColorBlack, GColorYellow);
	outlined_text_layer_set_font(time_layer, font_numbers);
	outlined_text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    
    date_layer = outlined_text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(5,0), bounds.size.w, 90));
	outlined_text_layer_set_colors(date_layer, GColorRed, GColorYellow);
	outlined_text_layer_set_font(date_layer, font_chinese);
    outlined_text_layer_set_text_overflow_mode(date_layer, GTextOverflowModeWordWrap);
	outlined_text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    outlined_text_layer_set_offset_x(date_layer, 3);
    
    week_layer = outlined_text_layer_create(GRect(5, bounds.size.h-PBL_IF_ROUND_ELSE(45,35), bounds.size.w-5, PBL_IF_ROUND_ELSE(45,35)));
	outlined_text_layer_set_colors(week_layer, GColorRed, GColorYellow);
	outlined_text_layer_set_font(week_layer, font_chinese);
	outlined_text_layer_set_text_alignment(week_layer, GTextAlignmentCenter);
    
    animal_layer = bitmap_layer_create(next_animal_position(bounds));
    bitmap_layer_set_compositing_mode(animal_layer, GCompOpSet);
    #define FU_SIZE_X 52
    #define FU_SIZE_Y 46
    fu_layer = bitmap_layer_create((GRect) {
        .origin={(bounds.size.w - FU_SIZE_X) /2, (bounds.size.h - FU_SIZE_Y) /2 },
        .size={ FU_SIZE_X , FU_SIZE_Y }
    });
    bitmap_layer_set_compositing_mode(fu_layer, GCompOpSet);
    fu = gbitmap_create_with_resource(RESOURCE_ID_FU);
    bitmap_layer_set_bitmap(fu_layer, fu);
    
    redsq_layer = layer_create(bounds);
    layer_set_update_proc(redsq_layer, draw_redsq);
    layer_set_hidden(redsq_layer, true);
    GRect lantern_rect = (GRect) {
          .origin={LANTERN_MARGIN_X, LANTERN_MARGIN_Y},
          .size={bounds.size.w - LANTERN_MARGIN_X*2, bounds.size.h - LANTERN_MARGIN_Y}
      };
    lantern_layer = layer_create(lantern_rect);
    layer_set_update_proc(lantern_layer, draw_lantern);
    strand_layer = layer_create(lantern_rect);
    layer_set_update_proc(strand_layer, draw_strands);
	
    
	//Add layers
    layer_add_child(window_layer, lantern_layer);
    layer_add_child(window_layer, strand_layer);
    layer_add_child(window_layer, redsq_layer);
    layer_add_child(window_layer, bitmap_layer_get_layer(animal_layer));
    layer_add_child(window_layer, bitmap_layer_get_layer(fu_layer));
	layer_add_child(window_layer, outlined_text_layer_get_layer(time_layer));
    layer_add_child(window_layer, outlined_text_layer_get_layer(date_layer));
    layer_add_child(window_layer, outlined_text_layer_get_layer(week_layer));

    accel_tap_service_subscribe(tap_handler);
    animate(8*1000, 0, &anim_impl, true);
}

static void main_window_unload(Window *w) {
    tick_timer_service_unsubscribe();
    connection_service_unsubscribe();
    battery_state_service_unsubscribe();
    bitmap_layer_destroy(fu_layer);
    gbitmap_destroy(fu);
    outlined_text_layer_destroy(time_layer);
    outlined_text_layer_destroy(date_layer);
    outlined_text_layer_destroy(week_layer);
    fonts_unload_custom_font(font_chinese);
    fonts_unload_custom_font(font_numbers);
    layer_destroy(strand_layer);
    layer_destroy(lantern_layer);
    layer_destroy(redsq_layer);
    gpath_destroy(redsq_path);
    if(animal_layer != NULL) {
        bitmap_layer_destroy(animal_layer);
        gbitmap_destroy(animal);
    }
}

static void init() {
    srand(time(NULL));
    // Create main Window element and assign to pointer
    main_window = window_create();
    
    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(main_window, (WindowHandlers) {
            .load = main_window_load,
            .unload = main_window_unload
    });

    // Show the Window on the watch, with animated=true
    window_stack_push(main_window, true);

    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_minute_handler);
}

static void deinit() {
    window_destroy(main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}