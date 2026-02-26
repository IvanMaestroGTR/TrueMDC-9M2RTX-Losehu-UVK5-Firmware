/* Copyright (c) 2026 James Honiball (KC3TFZ)
 * 
 * This file is part of VUURWERK and is dual-licensed:
 *   1. GPL v3 (when distributed as part of the VUURWERK firmware)
 *   2. Commercial license available from the author
 * 
 * You may not extract, repackage, or redistribute this file 
 * independently under any license other than GPL v3 as part 
 * of the complete VUURWERK firmware, without written permission
 * from the author.
 */

#ifndef SQUELCH_TAIL_H
#define SQUELCH_TAIL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	STE_IDLE,        // Not monitoring (no CTCSS active or not in RX)
	STE_MONITORING,  // CTCSS active, watching for tone loss
	STE_TONE_LOST,   // Tone disappeared, confirming (20ms)
	STE_MUTED,       // Audio muted, waiting for carrier to drop
} STE_State_t;

typedef struct {
	STE_State_t state;
	uint8_t     lost_count;   // Ticks since tone lost (10ms each)
	uint8_t     mute_count;   // Ticks since audio muted
	bool        trigger_end_tone;  // Flag to trigger end-call tone in STE mode
	bool        trigger_end_tone_nonstes;  // Flag to trigger end-call tone in non-STE mode
	uint8_t     end_tone_delay_count;  // Counter for 50ms delay before playing tone
} SquelchTail_t;

extern SquelchTail_t gSquelchTail;

void SQUELCH_TAIL_Init(void);
void SQUELCH_TAIL_Process(void);   // Called every 10ms, self-activates on RX with CTCSS
void SQUELCH_TAIL_PlayEndTone(void); // Plays the 1396Hz end-call tone for 150ms

#endif
