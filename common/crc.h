/// Syntacore SCR* infra
///
/// @copyright (C) Syntacore 2015-2017. All rights reserved.
/// @author mn-sc
///
/// @brief CRC routines

#include <stdint.h>

// Name  : CRC-16 CCITT
// Poly  : 0x1021    x^16 + x^12 + x^5 + 1
// Init  : 0xFFFF
// Revert: false
// XorOut: 0x0000
// Check : 0x29B1 ("123456789")
// MaxLen: 4095 bytes (32767 bit)

#define CRC16_INT16_UPD

static inline uint16_t crc16_ccitt_update(uint16_t crc, uint8_t data)
{
    int i;

    crc ^= (uint16_t)data << 8;
#ifdef CRC16_INT16_UPD
    for (i = 0; i < 8; ++i) {
        uint32_t tmp = (uint32_t)crc << 1;
        uint16_t sgn = ~((uint16_t)(tmp >> 16) - 1) & 0x1021;
        crc = (tmp & 0xffff) ^ sgn;
    }
#else
    for (i = 0; i < 8; i++) {
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    }
#endif
    return crc;
}
