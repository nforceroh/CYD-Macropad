#include <ArduinoJson.h>
#include <LittleFS.h>
#include "macropad.h"

// Global definitions (linked to macropad.h)
Macro* dynamic_macros[MAX_PAGES] = {NULL};
int dynamic_counts[MAX_PAGES] = {0};
char* page_titles[MAX_PAGES] = {NULL};
int page_rows[MAX_PAGES] = {0};
int page_cols[MAX_PAGES] = {0};
int* page_row_counts[MAX_PAGES] = {NULL};
int page_row_count_sizes[MAX_PAGES] = {0};
int actual_page_count = 0;
int auto_dim_seconds = DEFAULT_AUTO_DIM;
int auto_sleep_seconds = DEFAULT_AUTO_SLEEP;
int current_page_idx = 0;
const char* menu_paths[MAX_PAGES] = {
    "/menus/4x5/01.json", "/menus/4x5/02.json", "/menus/4x5/03.json",
    "/menus/4x5/01.json", "/menus/4x5/02.json"
};

// Theme globals
uint32_t theme_screen_bg, theme_status_bar, theme_button_bg, theme_button_border;
uint32_t theme_button_pressed, theme_text_main, theme_text_status;
uint32_t theme_conn = 0x00FF00, theme_disconn = 0xFF0000;

uint32_t hexToUint(const char* hex) {
    if (!hex || *hex == '\0') return 0;
    return (uint32_t)strtol((*hex == '#') ? hex + 1 : hex, NULL, 16);
}

bool loadConfig() {
    LOG_I("CFG", "Starting loadConfig...");

    if (!LittleFS.exists(CONFIG_FILE)) {
        LOG_E("CFG", "CRITICAL: config file not found: %s", CONFIG_FILE);
        return false;
    }

    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) {
        LOG_E("CFG", "Failed to open %s file handle", CONFIG_FILE);
        return false;
    }

    JsonDocument* doc = new JsonDocument();
    DeserializationError error = deserializeJson(*doc, f);
    f.close();

    if (error) {
        LOG_E("CFG", "JSON Parse Error: %s", error.c_str());
        delete doc;
        return false;
    }

    LOG_I("CFG", "Main config parsed.");

    auto_dim_seconds = (*doc)["auto_dim"] | DEFAULT_AUTO_DIM;
    auto_sleep_seconds = (*doc)["auto_sleep"] | DEFAULT_AUTO_SLEEP;

    const char* active = (*doc)["active_theme"] | "Dark";
    JsonArray themes = (*doc)["themes"];
    if (!themes.isNull()) {
        for (JsonObject t : themes) {
            const char* themeName = t["name"];
            if (themeName && strcmp(themeName, active) == 0) {
                theme_screen_bg = hexToUint(t["screen_bg"]);
                theme_status_bar = hexToUint(t["status_bar"]);
                theme_button_bg = hexToUint(t["button_bg"]);
                theme_button_border = hexToUint(t["button_border"]);
                theme_button_pressed = hexToUint(t["button_pressed"]);
                theme_text_main = hexToUint(t["text_main"]);
                theme_text_status = hexToUint(t["text_status"] | "#A0A0A0");
                LOG_I("CFG", "Theme '%s' applied.", active);
                break;
            }
        }
    }

    JsonArray pages = (*doc)["menus"];
    actual_page_count = 0;
    
    if (!pages.isNull()) {
        for (int p = 0; p < (int)pages.size() && p < MAX_PAGES; p++) {
            const char* path = pages[p];
            if (!path) continue;

            LOG_I("CFG", "Opening Page: %s", path);
            File pf = LittleFS.open(path, "r");
            if (!pf) {
                LOG_W("CFG", "Page file %s not found.", path);
                continue;
            }

            JsonDocument pdoc; // Individual page docs are small enough for stack
            if (deserializeJson(pdoc, pf) == DeserializationError::Ok) {
                page_titles[p] = strdup(pdoc["title"] | "Page");
                JsonArray rows = pdoc["rows"];
                
                if (!rows.isNull()) {
                    // record declared row/col dimensions for this page
                    int rcount = (int)rows.size();
                    int maxcols = 0;
                    int *counts = (int*)calloc(rcount, sizeof(int));
                    int ridx = 0;
                    for (JsonArray r : rows) {
                        int c = (int)r.size();
                        counts[ridx++] = c;
                        if (c > maxcols) maxcols = c;
                    }
                    page_rows[p] = rcount;
                    page_cols[p] = maxcols;
                    page_row_counts[p] = counts;
                    page_row_count_sizes[p] = rcount;

                    int total = 0;
                    for (JsonArray r : rows) total += r.size();
                    
                    dynamic_macros[p] = (Macro*)calloc(total, sizeof(Macro));
                    if (!dynamic_macros[p]) {
                        LOG_E("CFG", "MALLOC FAIL for page %d", p);
                        pf.close();
                        continue;
                    }

                    int idx = 0;
                    for (JsonArray r : rows) {
                        for (JsonObject b : r) {
                            Macro* m = &dynamic_macros[p][idx++];
                            strncpy(m->label, b["label"] | "Key", 31);
                            strncpy(m->keypress, b["keypress"] | "", 127);
                            strncpy(m->action, b["action"] | "keyboard", 15);
                            strncpy(m->payload, b["cmd"] | b["payload"] | "", 63);
                            strncpy(m->iconPath, b["img"] | "", 63);
                        }
                    }
                    dynamic_counts[p] = idx;
                    actual_page_count++;
                }
            } else {
                LOG_E("CFG", "Failed to parse page: %s", path);
            }
            pf.close();
        }
    }

    delete doc; // Cleanup heap memory
    LOG_I("CFG", "Load complete. Pages: %d", actual_page_count);
    return true;
}
