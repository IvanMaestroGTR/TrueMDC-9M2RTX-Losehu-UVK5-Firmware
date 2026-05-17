#ifdef ENABLE_FLASHLIGHT

#include "driver/gpio.h"
#include "bsp/dp32g030/gpio.h"

#include "flashlight.h"

enum FlashlightMode_t  gFlashLightState;

void FlashlightTimeSlice()
{
    if (gFlashLightState == FLASHLIGHT_BLINK) {
        if ((gFlashLightBlinkCounter & 15u) == 0) {
            GPIO_FlipBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
        }
    }
}

void ACTION_FlashLight(void)
{
	switch (gFlashLightState) {
		case FLASHLIGHT_OFF:
			gFlashLightState = FLASHLIGHT_ON;
			GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
			break;
		case FLASHLIGHT_ON:
			gFlashLightState = FLASHLIGHT_BLINK;
			break;
		case FLASHLIGHT_BLINK:
		default:
			gFlashLightState = FLASHLIGHT_OFF;
			GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);
			break;
	}
}

#endif