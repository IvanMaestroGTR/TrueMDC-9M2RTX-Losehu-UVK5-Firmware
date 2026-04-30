#ifndef TX_COMPRESSOR_H
#define TX_COMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool     enabled;
	uint8_t  threshold;      // 0-31
	uint8_t  ratio_x10;      // 20=2:1, 30=3:1, 40=4:1
	uint8_t  attack_ms;      // 2-20
	uint16_t release_ms;     // 50-1000
	uint8_t  makeup_gain;    // 0-10
	uint8_t  noise_gate_threshold;  // 0-31, signals below this are attenuated (noise removal)
} CompressorConfig_t;

extern CompressorConfig_t gCompressorConfig;

void TX_COMPRESSOR_Init(void);
void TX_COMPRESSOR_Start(void);    // Called once at TX start
void TX_COMPRESSOR_Process(void);  // Called every 10ms during TX — auto-calibrates voice level in 0.5 seconds
void TX_COMPRESSOR_Stop(void);     // Called at TX end — restores REG_7D
uint8_t TX_COMPRESSOR_GetGainReduction(void);  // For UI
void TX_COMPRESSOR_TriggerFeedbackSuppression(void);  // Manually trigger mic mute for 50ms to break feedback
uint16_t TX_COMPRESSOR_GetVoiceBaseline(void);  // Get learned voice baseline (for debugging)
bool TX_COMPRESSOR_IsCalibrationComplete(void);  // Check if 0.5-second calibration is complete

#endif // TX_COMPRESSOR_H
