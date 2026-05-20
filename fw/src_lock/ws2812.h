#ifndef __WS2812_H__
#define __WS2812_H__


uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

void led_onboard(uint32_t pixel_grb);
void led_rgb_onboard_init(uint pin);

void rgb_light(uint32_t light_grb);
void rgb_light_init(uint pin);

#endif
