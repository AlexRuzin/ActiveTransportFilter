#include <ntddk.h>

#include "crc.h"

//
// Take the CRC16 of a databuffer
//
UINT16 crc16(const VOID *buf, size_t len)
{
    static const UINT16 crc16_seed = CRC16_SEED;

    UINT16 crc = crc16_seed;
    UINT8 *ptr = (UINT8 *)buf;

    while (len--) {
        crc = (crc >> 4) ^ ((crc & 1) ? 0x0003 : 0);
        crc ^= *ptr;
        ptr++;

        if (crc & 0xff00) {
            crc = (crc << 4) | (crc >> 12);
        }    
    }

    return ~crc;
}

