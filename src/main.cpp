#include "macropad.h"
#include "ui_helper.h"
#include <LittleFS.h>

TFT_eSPI tft = TFT_eSPI();
BleKeyboard bleKeyboard("MacroPad", "Espressif", 100);

uint16_t *buf1 = nullptr, *buf2 = nullptr;
bool is_dimmed = false;
bool is_sleeping = false;

void setBacklight(uint8_t brightness) { 
    ledcWrite(TFT_BL, brightness); 
}

// --- PROTECTION & WAKE LOGIC ---

void enterModemSleep() {
    if (is_sleeping) return;
    LOG_I("SYS", "Screen Off (BLE Active)");
    is_sleeping = true;

    // 1. Send hardware sleep command to TFT (Stops pixel driving/prevents burn-in)
    tft.writecommand(0x10); // ST7789_SLPIN
    setBacklight(0);
    
    // Note: CPU and BLE remain ON here so you stay connected to your PC.
}

void wakeFromSleep() {
    if (!is_sleeping) return;
    LOG_I("SYS", "Waking Screen...");
    
    // 1. Wake TFT hardware
    tft.writecommand(0x11); // ST7789_SLPOUT
    delay(120);             // Required for driver stabilization
    
    // 2. Fade in
    for (int b = 0; b <= 200; b += 10) {
        setBacklight(b);
        delay(2);
    }

    is_sleeping = false;
    is_dimmed = false;
    lv_display_trigger_activity(NULL); 
}

void checkDisplayPowerManagement() {
    uint32_t idle_sec = lv_display_get_inactive_time(NULL) / 1000;

    // Handle Screen Off (Modem Sleep)
    if (auto_sleep_seconds > 0 && idle_sec > auto_sleep_seconds) {
        enterModemSleep();
    } 
    // Handle Auto Dim
    else if (auto_dim_seconds > 0 && idle_sec > auto_dim_seconds) {
        if (!is_dimmed && !is_sleeping) {
            setBacklight(20); 
            is_dimmed = true;
            LOG_I("SYS", "Auto-dim active");
        }
    }
}

// --- UPDATED TOUCH READER ---

void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y, 600)) {
        if (is_sleeping || is_dimmed) {
            wakeFromSleep(); // Restores backlight and de-sleeps TFT
            return;          // Swallow first touch to prevent accidental button press
        }
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x; data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// --- STANDARD LVGL FLUSH & SETUP ---

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
    tft.begin();
    tft.initDMA();
    tft.setRotation(TFT_ROTATION);
    ledcAttach(TFT_BL, 5000, 8);
    setBacklight(200);

    if (!LittleFS.begin()) LOG_E("FS", "LittleFS Failed");

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
}

void loop() {
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
