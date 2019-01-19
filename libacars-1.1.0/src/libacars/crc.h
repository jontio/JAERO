/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_CRC_ARINC_H
#define LA_CRC_ARINC_H

#include <stdint.h>
#include <stdbool.h>

bool la_check_crc16_arinc(uint8_t const *data, uint32_t len);
uint16_t la_crc16_ccitt(uint8_t const *data, uint32_t len, uint16_t const crc_init);

#endif // !LA_CRC_ARINC_H
