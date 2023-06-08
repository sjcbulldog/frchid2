#pragma once

#include <stdint.h>

extern int hid_init() ;
extern void hid_run(int16_t *data, uint32_t count, uint32_t buttons, uint8_t *outbuf, uint32_t *size) ;