#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <BleKeyboard.h>


#ifndef CONFIG_FILE
#define CONFIG_FILE "/config.json"
#endif

#ifndef MAX_PAGES
#define MAX_PAGES 5
#endif

#ifndef TOP_STATUS_BAR_HEIGHT
#define TOP_STATUS_BAR_HEIGHT 30
#endif

#define CALIBRATION_FILE "/touch.bin"
#define DEFAULT_AUTO_DIM 300
#define DEFAULT_AUTO_SLEEP 900

#define LOG_I(tag, fmt, ...) Serial.printf("[%s] INF: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) Serial.printf("[%s] ERR: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) Serial.printf("[%s] WRN: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) Serial.printf("[%s] DBG: " fmt "\n", tag, ##__VA_ARGS__)

struct Macro {
    char label[32];   
    char payload[64];
    char iconPath[64];
    char keypress[128];
    char action[16];
};

extern const char* menu_paths[MAX_PAGES];
extern int current_page_idx;

extern Macro* dynamic_macros[MAX_PAGES];
extern char* page_titles[MAX_PAGES];
extern int dynamic_counts[MAX_PAGES];
extern int page_rows[MAX_PAGES];
extern int page_cols[MAX_PAGES];
extern int* page_row_counts[MAX_PAGES];
extern int page_row_count_sizes[MAX_PAGES];
extern int actual_page_count;
extern TFT_eSPI tft;
extern BleKeyboard bleKeyboard;

extern uint32_t theme_screen_bg, theme_status_bar, theme_button_bg, theme_button_border;
extern uint32_t theme_button_pressed, theme_text_main, theme_text_status;
extern uint32_t theme_conn, theme_disconn;
extern int auto_dim_seconds; 
extern int auto_sleep_seconds; 

bool loadConfig();
void checkCalibration();
void setBacklight(uint8_t brightness);
void checkAutoDimAndSleep();
