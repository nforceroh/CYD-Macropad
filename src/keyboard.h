#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Arduino.h>

// --- HID Key Mapping ---
uint8_t getRawKey(String name);

// --- Media Key Execution (map-based) ---
void handle_media_key(String k);

// --- Keypress Execution (single or combo) ---
void executeKeypress(const String& k);

#endif // KEYBOARD_H
