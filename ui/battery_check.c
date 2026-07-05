#include <string.h>
#include "driver/eeprom.h"
#include "driver/st7565.h"
#include "driver/keyboard.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#include "audio.h"
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "ui/battery_check.h"
#include "ui/helper.h"
#include "misc.h"

// Global variables for battery check screen state
uint16_t gBatteryCheckTimer = 0;  // Timer in 10ms ticks (300 = 3 seconds)
bool gBatteryCheckTonesPlayed = false;  // Track if tones have been played
bool gBatteryCheckActive = false;  // Track if battery check screen is currently displayed

void UI_DisplayBatteryCheck(void) {
    char VoltageString[20] = {0};
    
    memset(gStatusLine, 0, sizeof(gStatusLine));
    UI_DisplayClear();
    ST7565_BlitStatusLine();
    ST7565_BlitFullScreen();
    
    // Display battery voltage in center
    // Format: "X.XVV" where X.XX is the voltage
    sprintf(VoltageString, "%u.%02uV",
            gBatteryVoltageAverage / 100,
            gBatteryVoltageAverage % 100);
    
    // Draw centered battery text
    UI_PrintStringSmall(VoltageString, 0, 127, 3);
}

// Function to play voltage-based tone sequence
// This should be called from main() BEFORE entering the main loop, NOT from APP_TimeSlice10ms
// because we use blocking SYSTEM_DelayMs calls
void BatteryCheck_PlayVoltageTonesIfNeeded(void) {
    if (!gBatteryCheckActive || gBatteryCheckTonesPlayed) {
        return;  // Don't play tones if screen not active or already played
    }
    
    // Determine which tone sequence to play based on battery voltage
    // gBatteryVoltageAverage scale: 0-1000 where 700=7.0V, 760=7.6V
    uint16_t freq1, freq2;
    
    if (gBatteryVoltageAverage > 760) {
        // Above 7.6V: C5 (523Hz) + G5 (784Hz)
        freq1 = 523;
        freq2 = 784;
    } else if (gBatteryVoltageAverage >= 700) {
        // 7.0-7.6V: C5 (523Hz) + E5 (659Hz)
        freq1 = 523;
        freq2 = 659;
    } else {
        // Below 7.0V: C5 (523Hz) + C5 (523Hz)
        freq1 = 523;
        freq2 = 523;
    }
    
    // Reduce speaker pop by turning off audio first and using lower gain
    AUDIO_AudioPathOff();
    SYSTEM_DelayMs(20);
    
    // Configure tone with reduced gain to minimize pop
    uint16_t MaxGainConfig = BK4819_REG_70_ENABLE_TONE1 | (30 << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN);
    
    // First tone: 250ms
    BK4819_PlayTone(freq1, true);
    BK4819_WriteRegister(BK4819_REG_70, MaxGainConfig);
    SYSTEM_DelayMs(2);
    AUDIO_AudioPathOn();
    SYSTEM_DelayMs(60);  // Settle time - allows audio circuit to stabilize
    BK4819_ExitTxMute();
    SYSTEM_DelayMs(200);
    BK4819_EnterTxMute();
    
    // Second tone: 250ms
    BK4819_PlayTone(freq2, true);
    BK4819_WriteRegister(BK4819_REG_70, MaxGainConfig);
    SYSTEM_DelayMs(2);
    BK4819_ExitTxMute();
    SYSTEM_DelayMs(200);
    BK4819_EnterTxMute();
    SYSTEM_DelayMs(20);
    
    AUDIO_AudioPathOff();
    SYSTEM_DelayMs(5);
    BK4819_TurnsOffTones_TurnsOnRX();
    
    gBatteryCheckTonesPlayed = true;
}

// Function to check for key press and handle transitions
// Called from main.c or app.c
bool BatteryCheck_CheckKeyAndTimer(void) {
    // Check if any key is pressed to skip this screen
    if (KEYBOARD_Poll() != KEY_INVALID) {
        return true;  // Signal to skip to next screen
    }
    
    // Check if 3 seconds have elapsed (300 * 10ms = 3000ms)
    if (gBatteryCheckTimer >= 300) {
        return true;  // Signal to move to next screen
    }
    
    return false;  // Stay on battery check screen
}
