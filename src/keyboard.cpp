#include "keyboard.h"
#include "macropad.h"
#include <map>

extern BleKeyboard bleKeyboard;  // Declared in main.cpp or macropad.h

// --- HID Mapping Logic ---
uint8_t getRawKey(String name) {
    name.trim();
    uint8_t raw = 0;
    if (name == "KEY_LEFT_CTRL")        raw = 0x80;
    else if (name == "KEY_LEFT_SHIFT")  raw = 0x81;
    else if (name == "KEY_LEFT_ALT")    raw = 0x82;
    else if (name == "KEY_LEFT_GUI")    raw = 0x83;
    else if (name == "KEY_RIGHT_CTRL")  raw = 0x84;
    else if (name == "KEY_RIGHT_SHIFT") raw = 0x85;
    else if (name == "KEY_RIGHT_ALT")   raw = 0x86;
    else if (name == "KEY_RIGHT_GUI")   raw = 0x87;
    else if (name == "KEY_F8")          raw = 0xC9;
    else if (name == "KEY_RETURN")      raw = 0xB0;
    else if (name == "KEY_ESC")         raw = 0xB1;
    else if (name == "KEY_TAB")         raw = 0xB3;
    else if (name == "KEY_BACKSPACE")   raw = 0xB2;
    else if (name.startsWith("KEY_NUM_")) {
        int num = name.substring(8).toInt();
        raw = (num == 0) ? 0xEA : 0xE1 + (num - 1);
    }
    else if (name.length() == 1) raw = (uint8_t)name[0];

    if (raw) LOG_D("HID", "Mapped %s to 0x%02X", name.c_str(), raw);
    return raw;
}

// --- Helper: Execute key combo or single key ---
void executeKeypress(const String& k) {
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

void handle_media_key(String k) {
    // Media key lookup structure
    struct MediaKeyMap { const char* name; const uint8_t* code; };
    static const MediaKeyMap mediaKeys[] = {
        {"KEY_MEDIA_MUTE",           KEY_MEDIA_MUTE},
        {"KEY_MEDIA_VOLUME_UP",      KEY_MEDIA_VOLUME_UP},
        {"KEY_MEDIA_VOLUME_DOWN",    KEY_MEDIA_VOLUME_DOWN},
        {"KEY_MEDIA_NEXT_TRACK",     KEY_MEDIA_NEXT_TRACK},
        {"KEY_MEDIA_PREVIOUS_TRACK", KEY_MEDIA_PREVIOUS_TRACK},
        {"KEY_MEDIA_PLAY_PAUSE",     KEY_MEDIA_PLAY_PAUSE},
        {"KEY_MEDIA_STOP",           KEY_MEDIA_STOP}
    };

    for (const auto& key : mediaKeys) {
        if (k.equals(key.name)) {
            bleKeyboard.write(key.code);
            LOG_I("BLE", "Media Key Sent: %s", k.c_str());
            return;
        }
    }
    LOG_W("BLE", "Unknown Media Key: %s", k.c_str());
}
