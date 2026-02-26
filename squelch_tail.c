/* Copyright (c) 2026 Ivan (9W2RTX)
 * TrueMDC Project
 */

#include "squelch_tail.h"
#include "driver/bk4819.h"
#include "audio.h"
#include "driver/system.h"
#include "functions.h"
#include "radio.h"
#include "settings.h"
#include "misc.h"

SquelchTail_t gSquelchTail = {
	.state      = STE_IDLE,
	.lost_count = 0,
	.mute_count = 0,
	.trigger_end_tone = false,
	.trigger_end_tone_nonstes = false,
	.end_tone_delay_count = 0
};

static uint16_t scale_freq(const uint16_t freq) {
	return (((uint32_t) freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}

void SQUELCH_TAIL_Init(void)
{
	gSquelchTail.state      = STE_IDLE;
	gSquelchTail.lost_count = 0;
	gSquelchTail.mute_count = 0;
	gSquelchTail.trigger_end_tone = false;
	gSquelchTail.trigger_end_tone_nonstes = false;
	gSquelchTail.end_tone_delay_count = 0;
}

// Play tone to speaker only (no TX) for 100ms
static void SQUELCH_TAIL_PlayToneLocal(const unsigned int tone_Hz, const unsigned int level)
{
	// Save current tone configuration
	uint16_t ToneConfig = BK4819_ReadRegister(BK4819_REG_71);
	
	// Set up for tone playback (speaker only, no TX)
	AUDIO_AudioPathOff();
//	SYSTEM_DelayMs(20);
	
	// Use BK4819_PlayTone for speaker-only output
	BK4819_PlayTone(tone_Hz, true);
	
	// Turn on speaker
	AUDIO_AudioPathOn();
//	SYSTEM_DelayMs(5);
	
	// Play for 150ms
	BK4819_ExitTxMute();
	SYSTEM_DelayMs(150);
	
	// Stop tone
	BK4819_EnterTxMute();
//	SYSTEM_DelayMs(20);
	
	// Restore RX mode
	AUDIO_AudioPathOff();
//	SYSTEM_DelayMs(5);
	BK4819_TurnsOffTones_TurnsOnRX();
//	SYSTEM_DelayMs(5);
	BK4819_WriteRegister(BK4819_REG_71, ToneConfig);
	
	if (gEnableSpeaker)
		AUDIO_AudioPathOn();
}

void SQUELCH_TAIL_PlayEndTone(void)
{
	// Don't play tone during monitor mode
	if (gCurrentFunction == FUNCTION_MONITOR)
		return;
	
	// Handle delay counter for both flags
	if ((gSquelchTail.trigger_end_tone || gSquelchTail.trigger_end_tone_nonstes) && gEeprom.END_CALL_TONE) {
		if (gSquelchTail.end_tone_delay_count < 1) {  // 1 * 10ms = 10ms delay
			gSquelchTail.end_tone_delay_count++;
			return;
		}
		

		SQUELCH_TAIL_PlayToneLocal(1568, 70);
		gSquelchTail.trigger_end_tone = false;
		gSquelchTail.trigger_end_tone_nonstes = false;
		gSquelchTail.end_tone_delay_count = 0;
	}
}

void SQUELCH_TAIL_Process(void)
{
	if (!gEeprom.TAIL_TONE_ELIMINATION)
		return;

	const bool in_rx = (gCurrentFunction == FUNCTION_RECEIVE ||
	                    gCurrentFunction == FUNCTION_INCOMING);

	// Self-activate: enter MONITORING when RX starts with CTCSS
	if (gSquelchTail.state == STE_IDLE) {
		if (in_rx && gRxVfo->pRX->CodeType == CODE_TYPE_CONTINUOUS_TONE) {
			gSquelchTail.state      = STE_MONITORING;
			gSquelchTail.lost_count = 0;
			gSquelchTail.mute_count = 0;
			gSquelchTail.trigger_end_tone = false;
		}
		return;
	}

	// Left RX while active — reset
	if (!in_rx) {
		// Stock code handles AF restore on squelch close
		gSquelchTail.state = STE_IDLE;
		return;
	}

	// Read CTCSS found bit from BK4819 REG_0C
	const bool tone_present = (BK4819_ReadRegister(BK4819_REG_0C) >> 1) & 1;

	switch (gSquelchTail.state) {
	case STE_MONITORING:
		if (!tone_present) {
			gSquelchTail.lost_count = 1;
			gSquelchTail.state = STE_TONE_LOST;
		}
		break;

	case STE_TONE_LOST:
		if (tone_present) {
			// False alarm — tone came back (flutter/fade)
			gSquelchTail.state = STE_MONITORING;
			break;
		}
		gSquelchTail.lost_count++;
		if (gSquelchTail.lost_count >= 2) {
			// 20ms confirmed — MUTE NOW before noise burst
			BK4819_SetAF(BK4819_AF_MUTE);
			gSquelchTail.state = STE_MUTED;
			gSquelchTail.mute_count = 0;
		}
		break;

	case STE_MUTED:
		gSquelchTail.mute_count++;
		if (tone_present && gSquelchTail.mute_count > 3) {
			// Tone came back during mute — unmute and resume monitoring
			RADIO_SetModulation(gRxVfo->Modulation);
			gSquelchTail.state = STE_MONITORING;
		}
		else if (gSquelchTail.mute_count >= 12) {
			// 120ms timeout — restore audio, return to idle
			// Must unmute: on repeaters carrier stays up,
			// APP_StartListening won't run again to restore AF
			RADIO_SetModulation(gRxVfo->Modulation);
			
			// Trigger end-call tone if enabled
			if (gEeprom.END_CALL_TONE) {
				gSquelchTail.trigger_end_tone = true;
			}
			
			gSquelchTail.state = STE_IDLE;
		}
		break;

	default:
		break;
	}
}
