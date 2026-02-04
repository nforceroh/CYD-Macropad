#include "macropad.h"
#include "ui_helper.h"
#include <LittleFS.h>

TFT_eSPI tft = TFT_eSPI();

BleKeyboard bleKeyboard("MacroPad", "Espressif", 100);
uint16_t *buf1 = nullptr, *buf2 = nullptr;
bool is_dimmed = false;

void setBacklight(uint8_t brightness) { 
    ledcWrite(TFT_BL, brightness); 
}

void checkAutoDimAndSleep() {
    uint32_t idle_ms = lv_display_get_inactive_time(NULL);
    uint32_t idle_sec = idle_ms / 1000;

    // Handle Deep Sleep
    if (auto_sleep_seconds > 0 && idle_sec > (uint32_t)auto_sleep_seconds) {
        LOG_I("SYS", "Deep sleep triggered");
        setBacklight(0);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0); 
        esp_deep_sleep_start();
    }

    // Handle Auto Dim (Entry Only)
    if (auto_dim_seconds > 0 && idle_sec > (uint32_t)auto_dim_seconds) {
        if (!is_dimmed) {
            setBacklight(20); 
            is_dimmed = true;
            LOG_I("SYS", "Auto-dim active");
        }
    }
}

void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)px_map);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y, 600)) {
        if (is_dimmed) {
            LOG_I("SYS", "Waking up display...");
            lv_display_trigger_activity(NULL); // Reset inactivity timer
            for (int b = 20; b <= 200; b += 5) {
                setBacklight(b);
                delay(2); 
            }
            is_dimmed = false;
            return; // Swallow touch
        }
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x; data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    LOG_I("SYS", "Starting MacroPad Debug Mode...");

    // Initialise Hardware (Display first)
    tft.begin();
    tft.initDMA();
    tft.setRotation(TFT_ROTATION);
    ledcAttach(TFT_BL, 5000, 8);
    setBacklight(200);
    LOG_I("SYS", "TFT Hardware Initialized.");

    // Initialise FileSystem
    if (!LittleFS.begin()) {
        LOG_E("FS", "LittleFS Mount Failed!");
    }

    // Initialise LVGL Core
    lv_init();
    lv_tick_set_cb((lv_tick_get_cb_t)millis);
    LOG_I("SYS", "LVGL Core Initialized.");

    // Load Data and Config
    loadConfig();
    checkCalibration();
    
    uint32_t b_pix = SCREEN_WIDTH * 40; 
    buf1 = (uint16_t *)heap_caps_malloc(b_pix * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    buf2 = (uint16_t *)heap_caps_malloc(b_pix * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    // Note: The buffer size here is in BYTES (b_pix * size of color)
    lv_display_set_buffers(disp, buf1, buf2, b_pix * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // 3. Setup LVGL Input (Touch)
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    
    // 4. Initialise your UI functions
    ui_init_status_bar(); // This creates the top bar
    ui_load_page(0);      // This creates the main content
    LOG_I("SYS", "UI Initialized.");

    bleKeyboard.begin();
    LOG_I("SYS", "Setup Complete.");
}

void loop() {
    lv_timer_handler();
    checkAutoDimAndSleep();
    
    static bool last_c = false;
    bool curr_c = bleKeyboard.isConnected();
    if(curr_c != last_c) { 
        update_bt_status(curr_c); 
        last_c = curr_c; 
    }
    delay(5);
}
