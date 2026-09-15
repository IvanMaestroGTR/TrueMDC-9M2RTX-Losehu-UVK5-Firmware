#ifndef FLEETSYNC_H
#define FLEETSYNC_H

#include <stdbool.h>
#include <stdint.h>

/* FleetSync II ANI is one 64-bit block after the 32-bit sync word. */
#define FLEETSYNC_PACKET_SIZE 12u

/* FleetSync carries a 12-bit unit offset from the displayed value 999. */
#define FLEETSYNC_FLEET_MIN 100u
#define FLEETSYNC_FLEET_MAX 349u
#define FLEETSYNC_UNIT_MIN  1000u
#define FLEETSYNC_UNIT_MAX  4999u

/*
 * Builds a FleetSync II ANI/PTT-ID burst.
 * end_of_transmission selects the EOT form; false selects BOT.
 */
unsigned int FleetSync_encode_ani(void *data, uint16_t fleet, uint16_t unit,
                                  bool end_of_transmission);

/*
 * Decode a FleetSync II ANI burst.
 * Returns true if successfully decoded, false otherwise.
 */
bool FleetSync_decode_ani(const uint8_t *data, uint16_t *fleet, uint16_t *unit);

#endif
