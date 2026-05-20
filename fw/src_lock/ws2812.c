#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 1
#define LED_NUM_PIXELS 11

#define LED_RGB_ONBOARD_PIO pio0
#define LED_RGB_ONBOARD_SM 0
#define LIGHT_RGB_PIO pio1
#define LIGHT_RGB_SM 0

void led_onboard(uint32_t pixel_grb) {
  pio_sm_put_blocking(LED_RGB_ONBOARD_PIO, LED_RGB_ONBOARD_SM, pixel_grb << 8u);
}

void rgb_light(uint32_t light_grb) {
  for (int i=0; i<LED_NUM_PIXELS; i++) {
    pio_sm_put_blocking(LIGHT_RGB_PIO, LIGHT_RGB_SM, light_grb << 8u);
  }
}

uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

void led_rgb_onboard_init(uint pin) {
  PIO pio = LED_RGB_ONBOARD_PIO;
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, LED_RGB_ONBOARD_SM, offset, pin, 800000, IS_RGBW);
}

void rgb_light_init(uint pin) {
  PIO pio = LIGHT_RGB_PIO;
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, LIGHT_RGB_SM, offset, pin, 800000, IS_RGBW);
}
