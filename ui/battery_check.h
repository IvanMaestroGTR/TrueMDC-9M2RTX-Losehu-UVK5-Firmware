#ifndef UI_BATTERY_CHECK_H
#define UI_BATTERY_CHECK_H

#include <stdint.h>
#include <stdbool.h>

// Global variables for battery check screen state
extern uint16_t gBatteryCheckTimer;
extern bool gBatteryCheckTonesPlayed;
extern bool gBatteryCheckActive;

void UI_DisplayBatteryCheck(void);
void BatteryCheck_PlayVoltageTonesIfNeeded(void);
bool BatteryCheck_CheckKeyAndTimer(void);

#endif
