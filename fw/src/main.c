#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"

#include "ws2812.pio.h"
#include "ws2812.h"

#define LED_GREEN_PIN 12
#define BUTTON_PIN 13
#define TRIGGER_PIN 14
#define SENSE_ADC_PIN 26

#define millis() (to_ms_since_boot(get_absolute_time()))

void init(void) {
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
  ws2812_init(16);
}

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false;
  uint32_t tLed = 0;

  sleep_ms(100);

  while (true) {
    uint32_t now = millis();

    if (led && ((now - tLed) >= 50)) {
      led = false;
      put_pixel(urgb_u32(0x00, 0x00, 0x00));
    }
    if (!led && ((now - tLed) > 1000)) {
      led = true;
      put_pixel(urgb_u32(0x00, 0x10, 0x00));
      printf("Hello World\r\n");
      tLed = now;
    }

    //led = !led;
    //gpio_put(LED_GREEN_PIN, led);
    //gpio_put(TRIGGER_PIN, led);
    //put_pixel(urgb_u32(0x00, led ? 0x10 : 0x00, 0x00));
    //sleep_ms(1000);
  }
}
