#include "macropad.h"
#include "ui_helper.h"
#include <LittleFS.h>

TFT_eSPI tft = TFT_eSPI();

BleKeyboard bleKeyboard("MacroPad", "Espressif", 100);
uint16_t *buf1 = nullptr, *buf2 = nullptr;

// State tracking (Declared as extern in macropad.h)
bool is_dimmed = false;
bool is_sleeping = false;

// Timeouts (Matched to your macropad.h defaults or config)
// Note: auto_sleep_seconds here is used for "Screen Off" (Modem Sleep)
// auto_dim_seconds is used for "Lower Brightness"

void setBacklight(uint8_t brightness) { 
    ledcWrite(TFT_BL, brightness); 
}

// --- POWER MANAGEMENT LOGIC ---

void enterModemSleep() {
    if (is_sleeping) return;
    LOG_I("SYS", "Entering Modem Sleep (Screen Off, BLE ON)");
    
    is_sleeping = true;
    
    // 1. De-energize pixels to prevent burn-in/persistence
    tft.writecommand(0x10); // ST7789_SLPIN
    
    // 2. Kill backlight
    setBacklight(0);
    
    // CPU stays awake, BLE stays connected.
}

void wakeFromSleep() {
    if (!is_sleeping && !is_dimmed) return;
    
    LOG_I("SYS", "Waking display hardware...");

    // 1. Wake the TFT controller
    tft.writecommand(0x11); // ST7789_SLPOUT
    delay(120);             // Crucial delay for hardware charge pumps
    
    // 2. Smooth fade back in
    for (int b = 0; b <= 200; b += 10) {
        setBacklight(b);
        delay(2);
    }

    is_sleeping = false;
    is_dimmed = false;
    
    // Reset LVGL inactivity timer so it doesn't immediately re-sleep
    lv_display_trigger_activity(NULL); 
}

void checkDisplayPowerManagement() {
    uint32_t idle_ms = lv_display_get_inactive_time(NULL);
    uint32_t idle_sec = idle_ms / 1000;

    // 1. Check Screen Off (Modem Sleep)
    if (auto_sleep_seconds > 0 && idle_sec > (uint32_t)auto_sleep_seconds) {
        if (!is_sleeping) enterModemSleep();
    }
    // 2. Check Auto Dim
    else if (auto_dim_seconds > 0 && idle_sec > (uint32_t)auto_dim_seconds) {
        if (!is_dimmed && !is_sleeping) {
            setBacklight(20); 
            is_dimmed = true;
            LOG_I("SYS", "Auto-dim active");
        }
    }
}

// --- UPDATED INPUT HANDLING ---

void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    uint16_t x, y;
    
    // Physically poll the touch screen
    if (tft.getTouch(&x, &y, 600)) {
        
        // If we are in any power-saving state, wake up first
        if (is_sleeping || is_dimmed) {
            // Reset the LVGL idle timer immediately
            lv_display_trigger_activity(NULL); 
            
            wakeFromSleep();
            
            // "Swallow" the touch so we don't trigger a button during the wake-up
            data->state = LV_INDEV_STATE_RELEASED;
            return; 
        }

        // Normal operation
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x; 
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// --- CORE LVGL & DISPLAY FUNCTIONS ---

void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)px_map);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    LOG_I("SYS", "Starting MacroPad...");

    // Hardware Init
    tft.begin();
    tft.initDMA();
    tft.setRotation(TFT_ROTATION);
    ledcAttach(TFT_BL, 5000, 8);
    setBacklight(200);

    if (!LittleFS.begin()) {
        LOG_E("FS", "LittleFS Mount Failed!");
    }

    // LVGL Init
    lv_init();
    lv_tick_set_cb((lv_tick_get_cb_t)millis);

    loadConfig();
    checkCalibration();
    
    uint32_t b_pix = SCREEN_WIDTH * 40; 
    buf1 = (uint16_t *)heap_caps_malloc(b_pix * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    buf2 = (uint16_t *)heap_caps_malloc(b_pix * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf1, buf2, b_pix * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    
    ui_init_status_bar(); 
    ui_load_page(0);      

    bleKeyboard.begin();
    LOG_I("SYS", "Setup Complete.");
}

void loop() {
    // Keep LVGL timers running so it can process the touch wake-up
    lv_timer_handler();
    
    checkDisplayPowerManagement();
    
    static bool last_c = false;
    bool curr_c = bleKeyboard.isConnected();
    if(curr_c != last_c) { 
        update_bt_status(curr_c); 
        last_c = curr_c; 
    }
    delay(5);
}
