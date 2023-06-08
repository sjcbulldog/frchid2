#pragma once

#include <stdint.h>

int hw_init() ;
void hw_read(int16_t *data, uint32_t count, uint32_t *buttons);
void hw_write(uint8_t *outbuf, uint32_t count) ;