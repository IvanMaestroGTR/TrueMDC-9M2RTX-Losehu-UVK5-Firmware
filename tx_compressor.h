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
} CompressorConfig_t;

extern CompressorConfig_t gCompressorConfig;

void TX_COMPRESSOR_Init(void);
void TX_COMPRESSOR_Start(void);    // Called once at TX start
void TX_COMPRESSOR_Process(void);  // Called every 10ms during TX
void TX_COMPRESSOR_Stop(void);     // Called at TX end — restores REG_7D
uint8_t TX_COMPRESSOR_GetGainReduction(void);  // For UI

#endif // TX_COMPRESSOR_H
