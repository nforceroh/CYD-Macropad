#include "macropad.h"
#include "ui_helper.h"
#include "keyboard.h"
#include <LittleFS.h>
#include <algorithm>
#include <cmath>
#include <climits>

lv_obj_t *tileview;
lv_obj_t * bt_led;
lv_obj_t * bt_label;
lv_obj_t * status_title;
static lv_style_t style_grid;

// --- Helper: Convert String to LVGL Symbol ---
const char* getSymbol(const char* label) {
    if (!label) return "";
    if (strcmp(label, "LV_SYMBOL_MUTE") == 0) return LV_SYMBOL_MUTE;
    if (strcmp(label, "LV_SYMBOL_VOLUME_MID") == 0) return LV_SYMBOL_VOLUME_MID;
    if (strcmp(label, "LV_SYMBOL_VOLUME_MAX") == 0) return LV_SYMBOL_VOLUME_MAX;
    if (strcmp(label, "LV_SYMBOL_PREV") == 0) return LV_SYMBOL_PREV;
    if (strcmp(label, "LV_SYMBOL_NEXT") == 0) return LV_SYMBOL_NEXT;
    if (strcmp(label, "LV_SYMBOL_LEFT") == 0) return LV_SYMBOL_LEFT;
    if (strcmp(label, "LV_SYMBOL_RIGHT") == 0) return LV_SYMBOL_RIGHT;
    if (strcmp(label, "LV_SYMBOL_HOME") == 0) return LV_SYMBOL_HOME;
    if (strcmp(label, "LV_SYMBOL_PLAY") == 0) return LV_SYMBOL_PLAY;
    if (strcmp(label, "LV_SYMBOL_PAUSE") == 0) return LV_SYMBOL_PAUSE;
    if (strcmp(label, "LV_SYMBOL_SETTINGS") == 0) return LV_SYMBOL_SETTINGS;
    if (strcmp(label, "LV_SYMBOL_POWER") == 0) return LV_SYMBOL_POWER;
    return label; 
}

void update_bt_status(bool connected) {
    if (!bt_led || !bt_label) return;
    LOG_I("UI", "Bluetooth %s", connected ? "Connected" : "Disconnected");
    if (bt_led) {
        lv_led_set_color(bt_led, lv_color_hex(connected ? theme_conn : theme_disconn));
    }
    if (bt_label) {
        lv_label_set_text(bt_label, connected ? "Connected" : "Searching...");
        lv_obj_set_style_text_color(bt_label, lv_color_hex(connected ? theme_conn : theme_disconn), 0);
    }
}

// --- Button Action Handler (Keyboard Only) ---
static void btn_action_cb(lv_event_t * e) {
    Macro * m = (Macro *)lv_event_get_user_data(e);
    if (!m) return;

    if (!bleKeyboard.isConnected()) return;

    String k = String(m->keypress);
    
    if (k.startsWith("KEY_MEDIA_")) {
        handle_media_key(k);
    } else {
        executeKeypress(k);
    }
}

// --- Navigation Callback with Wrap-Around ---
static void nav_click_cb(lv_event_t * e) {
    int dir = (intptr_t)lv_event_get_user_data(e);
    int next = current_page_idx + dir;

    if (next < 0) next = actual_page_count - 1;
    else if (next >= actual_page_count) next = 0;

    ui_load_page(next);
}

static void btnm_event_cb(lv_event_t * e) {
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
    
    // Get the index of the clicked button
    uint32_t btn_id = lv_buttonmatrix_get_selected_button(obj);

    // Check if a valid button was actually selected
    if(btn_id == LV_BUTTONMATRIX_BUTTON_NONE) return;

    // Use the ID to get the macro from your dynamic_macros array
    Macro * m = &dynamic_macros[current_page_idx][btn_id];
    LOG_D("UI", "Button %d clicked: %s", btn_id, m->label);

    // Execute the keypress
    if (!bleKeyboard.isConnected()) {
        LOG_W("UI", "BLE not connected");
        return;
    }

    String k = String(m->keypress);
    
    if (k.startsWith("KEY_MEDIA_")) {
        handle_media_key(k);
    } else {
        executeKeypress(k);
    }
}

void ui_init_status_bar() {
    lv_obj_t * top = lv_layer_top();
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    // --- Container ---
    lv_obj_t * bar = lv_obj_create(top);
    lv_obj_set_size(bar, LV_HOR_RES, TOP_STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(theme_status_bar), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // --- Bluetooth status ---
    bt_led = lv_led_create(bar);
    lv_obj_set_size(bt_led, 10, 10);
    lv_obj_align(bt_led, LV_ALIGN_LEFT_MID, 10, 0);
    lv_led_set_color(bt_led, lv_color_hex(theme_disconn));

    bt_label = lv_label_create(bar);
    lv_label_set_text(bt_label, "Searching...");
    lv_obj_set_style_text_font(bt_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(bt_label, lv_color_hex(theme_text_status), 0);
    lv_obj_align(bt_label, LV_ALIGN_LEFT_MID, 25, 0);

    // --- Dynamic Title ---
    status_title = lv_label_create(bar);
    lv_label_set_text(status_title, "MacroPad");
    lv_obj_set_style_text_color(status_title, lv_color_hex(theme_text_main), 0);
    lv_obj_center(status_title);

    // --- Navigation Helper ---
    auto create_nav = [&](const char* sym, int dir, int x) {
        lv_obj_t * b = lv_btn_create(bar);
        lv_obj_set_size(b, 25, 25);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, x, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(theme_button_bg), 0);
        lv_obj_add_event_cb(b, nav_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)dir);
        lv_obj_t * l = lv_label_create(b);
        lv_label_set_text(l, sym);
        lv_obj_center(l);
    };

    create_nav(LV_SYMBOL_RIGHT, 1, -5);
    
    // Home
    lv_obj_t * h = lv_btn_create(bar);
    lv_obj_set_size(h, 25, 25);
    lv_obj_align(h, LV_ALIGN_RIGHT_MID, -45, 0);
    lv_obj_add_event_cb(h, [](lv_event_t* e){ ui_load_page(0); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t * hl = lv_label_create(h);
    lv_label_set_text(hl, LV_SYMBOL_HOME);
    lv_obj_center(hl);

    create_nav(LV_SYMBOL_LEFT, -1, -85);
}

void ui_load_page(int index) {
    LOG_D("UI_MAT", "Loading page index: %d", index);

    if (index < 0 || index >= actual_page_count) {
        LOG_E("UI_MAT", "Index out of bounds! Max: %d", actual_page_count);
        return;
    }
    current_page_idx = index;

    // 1. Create the Main Screen
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(theme_screen_bg), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0); 
    LOG_D("UI_MAT", "Screen created with color: 0x%06X", theme_screen_bg);

    // 2. Prepare the Button Map
    static const char* active_map[40]; 
    int map_ptr = 0;
    for (int i = 0; i < dynamic_counts[index] && map_ptr < 39; i++) {
        const char *label = dynamic_macros[index][i].label;
        bool isSymbol = (label && strstr(label, "LV_SYMBOL_") != NULL);

        if (isSymbol) {
            active_map[map_ptr++] = getSymbol(label);
        } else {
            active_map[map_ptr++] = (label && label[0] != '\0') ? label : " ";
        }
        if ((i + 1) % 5 == 0 && (i + 1) < dynamic_counts[index] && map_ptr < 39) {
            active_map[map_ptr++] = "\n";
        }
    }
    active_map[map_ptr] = "";
    LOG_D("UI_MAT", "Map generated with %d buttons", dynamic_counts[index]);

    // 3. Create the Work Area
    lv_obj_t * work_area = lv_obj_create(scr);
    lv_obj_set_size(work_area, LV_HOR_RES, LV_VER_RES - TOP_STATUS_BAR_HEIGHT);
    lv_obj_align(work_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(work_area, 0, 0);
    lv_obj_set_style_border_width(work_area, 0, 0);
    lv_obj_set_style_pad_all(work_area, 0, 0);
    lv_obj_clear_flag(work_area, LV_OBJ_FLAG_SCROLLABLE);

    // 4. Create the Button Matrix
    lv_obj_t * btnm = lv_buttonmatrix_create(work_area);
    lv_buttonmatrix_set_map(btnm, active_map);
    lv_obj_set_size(btnm, lv_pct(100), lv_pct(100));

    // --- LVGL 9.4 CRITICAL FIX & DEBUG ---
    // This flag tells LVGL to actually fire the LV_EVENT_DRAW_TASK_ADDED event
    lv_obj_add_event_cb(btnm, btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

   
    // 5. Matrix Styling
    lv_obj_set_style_pad_all(btnm, 0, LV_PART_MAIN);     
    lv_obj_set_style_pad_gap(btnm, 1, LV_PART_MAIN);    
    lv_obj_set_style_bg_opa(btnm, 0, 0);                

    lv_obj_set_style_bg_color(btnm, lv_color_hex(theme_button_bg), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(btnm, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(btnm, lv_color_hex(theme_text_main), LV_PART_ITEMS);
    lv_obj_set_style_text_opa(btnm, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_radius(btnm, 0, LV_PART_ITEMS);    
    lv_obj_set_style_border_width(btnm, 0, LV_PART_ITEMS);

    // 6. Title Update
    if (status_title && page_titles[index]) {
        lv_label_set_text(status_title, page_titles[index]);
        LOG_D("UI_MAT", "Status title updated to: %s", page_titles[index]);
    }

    // 7. Transition
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
    
    // Force immediate redraw to test the callback
    lv_obj_invalidate(btnm);
    LOG_D("UI_MAT", "Transition started and Matrix invalidated for redraw");
}

// --- Navigation Logic ---
void change_page(const char* direction) {
    if (!tileview) return;
    uint32_t current = lv_obj_get_index(lv_tileview_get_tile_active(tileview));
    uint32_t next = current;

    if (strcmp(direction, "NEXT") == 0) {
        next = (current + 1 < (uint32_t)actual_page_count) ? current + 1 : 0;
    } else if (strcmp(direction, "PREV") == 0) {
        next = (current > 0) ? current - 1 : (uint32_t)actual_page_count - 1;
    }

    if (next != current) {
        lv_obj_set_tile_id(tileview, next, 0, LV_ANIM_ON);
        if(status_title && page_titles[next]) {
            lv_label_set_text(status_title, page_titles[next]);
        }
    }
}

static void nav_cb(lv_event_t * e) {
    const char* cmd = (const char*)lv_event_get_user_data(e);
    if (cmd) change_page(cmd);
}

// --- Button Interaction ---
static void btn_cb(lv_event_t * e) {
    Macro* m = (Macro*)lv_event_get_user_data(e);
    if (!m) return;
    
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        if (strcmp(m->action, "internal") == 0) {
            if (code == LV_EVENT_CLICKED) {
                if (strcmp(m->payload, "CMD_NEXT_PAGE") == 0) change_page("NEXT");
                else if (strcmp(m->payload, "CMD_PREV_PAGE") == 0) change_page("PREV");
            }
            return;
        }

        if (!bleKeyboard.isConnected()) return;

        String k = String(m->keypress);
        if (code == LV_EVENT_LONG_PRESSED_REPEAT && !k.startsWith("KEY_MEDIA_VOLUME")) return;

        if (k.startsWith("KEY_MEDIA_")) {
            handle_media_key(k);
        } 
        else if (code == LV_EVENT_CLICKED) { 
            if (k.indexOf('+') != -1) {
                int last = 0, pos;
                while ((pos = k.indexOf('+', last)) != -1) {
                    uint8_t raw = getRawKey(k.substring(last, pos));
                    if (raw) bleKeyboard.press(raw);
                    last = pos + 1;
                }
                uint8_t final = getRawKey(k.substring(last));
                if (final) bleKeyboard.press(final);
                delay(50); bleKeyboard.releaseAll();
            } else {
                uint8_t raw = getRawKey(k);
                if (raw) bleKeyboard.write(raw);
                else bleKeyboard.print(k);
            }
        }
    }
}

