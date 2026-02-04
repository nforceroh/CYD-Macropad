#include <LittleFS.h>
#include "macropad.h"

void checkCalibration() {
    uint16_t calData[5];
    bool calDataOK = false;

    if (LittleFS.exists(CALIBRATION_FILE)) {
        File f = LittleFS.open(CALIBRATION_FILE, "r");
        if (f.readBytes((char *)calData, 10) == 10) {
            calDataOK = true;
            LOG_I("TOUCH", "Calibration Loaded.");
        }
        f.close();
    }

    if (calDataOK) { tft.setTouch(calData); } 
    else {
        LOG_W("TOUCH", "Starting Interactive Calibration...");
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString("TOUCH CALIBRATION", 240, 110);
        tft.setTextSize(1);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("1. Touch magenta corners as they appear.", 240, 150);
        tft.drawString("2. Hold until point vanishes.", 240, 170);
        delay(3000);
        tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
        
        File f = LittleFS.open(CALIBRATION_FILE, "w");
        f.write((const unsigned char *)calData, 10);
        f.close();
        tft.setTouch(calData);
        tft.fillScreen(TFT_BLACK);
        LOG_I("TOUCH", "Saved.");
    }
}
