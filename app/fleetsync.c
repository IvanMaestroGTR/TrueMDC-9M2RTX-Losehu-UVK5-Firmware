#include "fleetsync.h"

#include <stdint.h>

/* FleetSync II's 32-bit over-the-air sync. */
static const uint8_t fleetsync_sync[] = {0xaa, 0xaa, 0x23, 0xeb};

/*
 * This is the CRC-15 calculation used by the supplied decoder.  The returned
 * low bit is reserved for the block's even-parity check bit.
 */
static uint16_t FleetSync_crc15(const uint8_t *data)
{
    uint16_t crc = 0;

    for (unsigned int byte = 0; byte < 6; byte++) {
        for (int bit = 7; bit >= 0; bit--) {
            const uint16_t input = (data[byte] >> bit) & 1u;
            const uint16_t feedback = input ^ ((crc >> 15) & 1u);

            if (feedback)
                crc ^= 0x6815u;
            crc <<= 1;
        }
    }

    return crc ^ 0x0002u;
}

static uint8_t FleetSync_even_parity(const uint8_t *data, unsigned int size)
{
    uint8_t parity = 0;

    for (unsigned int i = 0; i < size; i++) {
        uint8_t value = data[i];
        while (value) {
            parity ^= 1u;
            value &= value - 1u;
        }
    }

    return parity;
}

static inline bool FleetSync_decode_block(const uint8_t *block, uint16_t *fleet, uint16_t *unit)
{
    const uint16_t crc_calculated = FleetSync_crc15(block);
    const uint16_t crc_received = ((uint16_t)block[6] << 8) | block[7];

    if ((crc_calculated >> 1) != (crc_received >> 1))
        return false;
    if (FleetSync_even_parity(block, 8) != 0)
        return false;

    *fleet = (uint16_t)block[2] + FLEETSYNC_FLEET_MIN;
    *unit = (uint16_t)(((uint16_t)block[3] << 4) | ((uint16_t)block[4] >> 4)) +
            FLEETSYNC_UNIT_MIN;

    return *fleet <= FLEETSYNC_FLEET_MAX && *unit <= FLEETSYNC_UNIT_MAX;
}

unsigned int FleetSync_encode_ani(void *data, uint16_t fleet, uint16_t unit,
                                  bool end_of_transmission)
{
    uint8_t *packet = data;
    uint8_t *block = packet + sizeof(fleetsync_sync);
    uint16_t fleet_raw = fleet - FLEETSYNC_FLEET_MIN;
    uint16_t unit_raw = unit - FLEETSYNC_UNIT_MIN;
    uint16_t crc;

    /* Callers normally validate these.  Clamping also makes this safe alone. */
    if (fleet < FLEETSYNC_FLEET_MIN || fleet > FLEETSYNC_FLEET_MAX)
        fleet_raw = 1;
    if (unit < FLEETSYNC_UNIT_MIN || unit > FLEETSYNC_UNIT_MAX)
        unit_raw = 1;

    for (unsigned int i = 0; i < sizeof(fleetsync_sync); i++)
        packet[i] = fleetsync_sync[i];

    /*
     * ANI, no Fleet-extension: the destination is the unassigned unit/fleet.
     * Bit 1 is the inverted EOT flag (1 = BOT, 0 = EOT).
     */
    block[0] = end_of_transmission ? 0x00 : 0x02;
    block[1] = 0x80;                    /* ANI flag */
    block[2] = (uint8_t)fleet_raw;
    block[3] = (uint8_t)(unit_raw >> 4);
    block[4] = (uint8_t)(unit_raw << 4);
    block[5] = 0x00;

    crc = FleetSync_crc15(block);
    block[6] = (uint8_t)(crc >> 8);
    block[7] = (uint8_t)crc;

    /* The least-significant bit of the final byte is the even parity bit. */
    block[7] &= 0xfeu;
    if (FleetSync_even_parity(block, 8) != 0)
        block[7] |= 1u;

    return FLEETSYNC_PACKET_SIZE;
}

/*
 * Decode a FleetSync II ANI burst.
 * Returns true if successfully decoded, false otherwise.
 */
bool FleetSync_decode_ani(const uint8_t *data, uint16_t *fleet, uint16_t *unit)
{
    if (!data || !fleet || !unit)
        return false;
    
    // Check sync word
    if (data[0] != 0xaa || data[1] != 0xaa || data[2] != 0x23 || data[3] != 0xeb)
        return false;
    
    const uint8_t *block = data + sizeof(fleetsync_sync);

    return FleetSync_decode_block(block, fleet, unit);
}
