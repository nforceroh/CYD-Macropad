#pragma once
#include <lvgl.h>

// --- Life Cycle ---
void ui_init();
void ui_init_status_bar();
void ui_load_page(int index); // default loader (calls flex)
void ui_load_page_flex(int index); // flex-based layout
void ui_load_page_old(int index); // absolute/button-matrix layout
void update_bt_status(bool connected); 
static void btnm_draw_event_cb(lv_event_t * e);
static void btnm_event_cb(lv_event_t * e);
static void nav_click_cb(lv_event_t * e);
