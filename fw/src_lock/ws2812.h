#ifndef __WS2812_H__
#define __WS2812_H__

void put_pixel(uint32_t pixel_grb);
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void ws2812_init(uint pin);

#endif
