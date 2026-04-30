#include "tx_compressor.h"
#include "driver/bk4819.h"
#include "radio.h"

// Configuration (user-adjustable via menu in future)
CompressorConfig_t gCompressorConfig = {
	.enabled     = true,
	.threshold   = 16,    // 0-31 mic level where compression starts (aggressive: catch early)
	.ratio_x10   = 60,    // 60 = 6:1 compression ratio (aggressive squashing for feedback)
	.attack_ms   = 3,     // Fast attack catches feedback peaks immediately
	.release_ms  = 150,   // Faster release for natural tracking without pumping
	.makeup_gain = 20,     // Higher post-compression boost to compensate
	.noise_gate_threshold = 4   // 0-31, signals below this are heavily attenuated (very aggressive: catch background noise early)
};

// Feedback suppression configuration
#define FEEDBACK_DETECTION_THRESHOLD 28  // Amplitude above this triggers feedback detection
#define FEEDBACK_MIC_MUTE_DURATION 50    // Mute mic for 50ms when feedback detected (breaks loop)

// Voice calibration
#define VOICE_CALIBRATION_SAMPLES 50     // ~0.5 seconds of voice samples at 10ms interval (fast detection)
#define VOICE_CALIBRATION_MULTIPLIER 1.3 // Scale factor: gate threshold = voice_baseline * 1.3 (30% margin below voice)

static uint16_t original_reg_7d;
static uint8_t  base_gain;
static bool     compressor_active = false;
static uint16_t feedback_mute_counter = 0;  // Countdown to restore mic after feedback mute
static uint16_t previous_mic_level = 0;    // Track amplitude changes for feedback detection
static uint16_t voice_baseline_level = 8;  // Learned voice baseline level, default conservative
static uint8_t  calibration_sample_count = 0;
static bool     calibration_complete = false;

// Envelope follower state
static uint32_t rms_accumulator = 0;
static uint8_t  rms_count = 0;
static uint16_t rms_level = 0;
static uint32_t envelope = 0;        // Fixed-point (<<8)
static bool     noise_gate_state = true;  // Gate open by default, closed when detecting noise
static uint8_t  gate_hold_count = 0;     // Hold gate closed for a bit to prevent fluttering

#define RMS_WINDOW      4            // 4 samples = 40ms at 10ms/tick
#define ENVELOPE_SHIFT  8
#define MIC_GAIN_MIN    4
#define MIC_GAIN_MAX    31
#define MIC_GAIN_MASK   0x1F
#define MIC_LEVEL_REG   0x64

// Integer square root (Newton's method)
static uint16_t isqrt32(uint32_t n) {
	if (n == 0) return 0;
	uint32_t x = n;
	uint32_t y = (x + 1) >> 1;
	while (y < x) { x = y; y = (x + n / x) >> 1; }
	return (uint16_t)(x > 0xFFFF ? 0xFFFF : x);
}

void TX_COMPRESSOR_Init(void) {
	compressor_active = false;
}

void TX_COMPRESSOR_Start(void) {
	if (!gCompressorConfig.enabled) return;

	original_reg_7d = BK4819_ReadRegister(BK4819_REG_7D);
	base_gain = original_reg_7d & MIC_GAIN_MASK;

	rms_accumulator = 0;
	rms_count = 0;
	rms_level = 0;
	envelope = 0;
	noise_gate_state = true;  // Gate open
	gate_hold_count = 0;
	feedback_mute_counter = 0;
	previous_mic_level = 0;
	calibration_sample_count = 0;
	calibration_complete = false;
	compressor_active = true;
}

void TX_COMPRESSOR_Process(void) {
	if (!compressor_active || !gCompressorConfig.enabled) return;

	// Decrement feedback mute counter if active
	if (feedback_mute_counter > 0) {
		feedback_mute_counter--;
		if (feedback_mute_counter > 0) {
			// Still muting - suppress gain to cut through feedback
			extern bool gMuteMic;
			gMuteMic = true;
			return;  // Skip normal processing while feedback muting
		}
	}

	// Step 1: Read mic amplitude
	uint16_t mic_raw = BK4819_ReadRegister(MIC_LEVEL_REG) & 0x7FFF;

	// Step 1.5: Voice level calibration (first 2 seconds)
	if (!calibration_complete) {
		calibration_sample_count++;
		
		// Accumulate voice baseline (only count non-silent periods)
		if (mic_raw > 5) {  // Skip pure noise floor
			if (calibration_sample_count == 1) {
				voice_baseline_level = mic_raw;
			} else {
				// Use sliding average to smooth the baseline
				voice_baseline_level = (voice_baseline_level * 3 + mic_raw) / 4;
			}
		}
		
		// Calibration complete after 200 samples (~2 seconds)
		if (calibration_sample_count >= VOICE_CALIBRATION_SAMPLES) {
			calibration_complete = true;
			// Set adaptive gate threshold: 30% below voice baseline
			uint8_t adaptive_threshold = (uint8_t)((voice_baseline_level * 7) / 10);  // 70% of baseline
			if (adaptive_threshold < 2) adaptive_threshold = 2;   // Minimum safety threshold
			gCompressorConfig.noise_gate_threshold = adaptive_threshold;
		}
	}

	// Step 1.6: Feedback detection - detect sharp spikes characteristic of feedback
	if (mic_raw > FEEDBACK_DETECTION_THRESHOLD) {
		// Sudden high amplitude detected - likely feedback
		feedback_mute_counter = (FEEDBACK_MIC_MUTE_DURATION / 10) + 1;  // Convert ms to 10ms ticks
	}

	previous_mic_level = mic_raw;

	// Step 2: RMS calculation (rolling window)
	uint16_t scaled = mic_raw >> 4;  // prevent uint32 overflow
	rms_accumulator += (uint32_t)scaled * scaled;
	rms_count++;

	if (rms_count >= RMS_WINDOW) {
		rms_level = isqrt32(rms_accumulator / RMS_WINDOW);
		rms_accumulator = 0;
		rms_count = 0;
	}

	// Step 3: Envelope follower (attack/release asymmetry)
	uint32_t rms_shifted = (uint32_t)rms_level << ENVELOPE_SHIFT;

	if (rms_shifted > envelope) {
		// ATTACK: fast
		uint8_t attack_ticks = gCompressorConfig.attack_ms / 10;
		if (attack_ticks < 1) attack_ticks = 1;
		uint32_t coeff = 256 / attack_ticks;
		envelope += ((rms_shifted - envelope) * coeff) >> 8;
	} else {
		// RELEASE: slow
		uint16_t release_ticks = gCompressorConfig.release_ms / 10;
		if (release_ticks < 1) release_ticks = 1;
		uint32_t coeff = 256 / release_ticks;
		envelope -= ((envelope - rms_shifted) * coeff) >> 8;
	}

	uint16_t env_actual = (uint16_t)(envelope >> ENVELOPE_SHIFT);

	// Step 4: Noise gate with hysteresis - suppress signals below calibrated voice baseline
	uint8_t noise_gate_reduction = 0;
	
	// Hysteresis: gate closes at threshold, opens at threshold + 1 (tighter control with calibration)
	uint8_t gate_open_threshold = gCompressorConfig.noise_gate_threshold + 1;
	
	if (env_actual < gCompressorConfig.noise_gate_threshold) {
		// Signal dropped below close threshold - close the gate
		noise_gate_state = false;
		gate_hold_count = 2;  // Hold gate closed for 20ms with tighter calibration
	} else if (env_actual > gate_open_threshold && gate_hold_count == 0) {
		// Signal rose above open threshold and hold time expired - open the gate
		noise_gate_state = true;
	}
	
	// Decrement hold counter
	if (gate_hold_count > 0) {
		gate_hold_count--;
	}
	
	// Apply gate reduction if closed
	if (!noise_gate_state) {
		noise_gate_reduction = 31;  // ~31dB reduction for background noise (maximum aggressive)
	}

	// Step 5: Compute compression gain reduction
	uint8_t compression_reduction = 0;
	if (env_actual > gCompressorConfig.threshold) {
		uint16_t excess = env_actual - gCompressorConfig.threshold;
		// reduction = excess * (1 - 10/ratio)
		uint16_t red_x10 = (excess * (gCompressorConfig.ratio_x10 - 10)) / gCompressorConfig.ratio_x10;
		compression_reduction = (uint8_t)(red_x10 >> 1);  // scale to gain steps
		if (compression_reduction > 15) compression_reduction = 15;
	}

	// Combined: noise gate + compressor reduction
	uint8_t total_reduction = (noise_gate_reduction > 0) ? noise_gate_reduction : compression_reduction;

	// Step 6: Apply to REG_7D
	int16_t final_gain = (int16_t)base_gain - total_reduction + gCompressorConfig.makeup_gain;
	if (final_gain < MIC_GAIN_MIN) final_gain = MIC_GAIN_MIN;
	if (final_gain > MIC_GAIN_MAX) final_gain = MIC_GAIN_MAX;

	uint16_t new_7d = (original_reg_7d & 0xFFE0) | ((uint16_t)final_gain & MIC_GAIN_MASK);
	BK4819_WriteRegister(BK4819_REG_7D, new_7d);
}

void TX_COMPRESSOR_Stop(void) {
	if (!compressor_active) return;
	BK4819_WriteRegister(BK4819_REG_7D, original_reg_7d);
	
	// Restore gMuteMic if it was suppressed
	extern bool gMuteMic;
	gMuteMic = false;  // Re-enable mic
	
	compressor_active = false;
}

uint8_t TX_COMPRESSOR_GetGainReduction(void) {
	// For UI display: current gain reduction in steps
	if (!compressor_active) return 0;
	
	// Noise gate takes priority
	if (!noise_gate_state) {
		return 31;  // Noise gate reduction (maximum aggressive)
	}
	
	// Otherwise show compression reduction
	uint16_t env_actual = (uint16_t)(envelope >> ENVELOPE_SHIFT);
	if (env_actual <= gCompressorConfig.threshold) return 0;
	uint16_t excess = env_actual - gCompressorConfig.threshold;
	uint16_t red = (excess * (gCompressorConfig.ratio_x10 - 10)) / gCompressorConfig.ratio_x10;
	uint8_t gr = (uint8_t)(red >> 1);
	return (gr > 15) ? 15 : gr;
}

// Trigger feedback suppression manually (useful for post-transmission muting)
void TX_COMPRESSOR_TriggerFeedbackSuppression(void) {
	feedback_mute_counter = (FEEDBACK_MIC_MUTE_DURATION / 10) + 1;
}

// Get current voice baseline level (for debugging/UI)
uint16_t TX_COMPRESSOR_GetVoiceBaseline(void) {
	return voice_baseline_level;
}

// Check if voice calibration is complete
bool TX_COMPRESSOR_IsCalibrationComplete(void) {
	return calibration_complete;
}
