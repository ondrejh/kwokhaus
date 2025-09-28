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

  sleep_ms(2000);           // Krátká prodleva, aby se terminál připojil
  while (true) {
    sleep_ms(1000);       // případně další logika
    printf("Hello World\r\n");
    led = !led;
    gpio_put(LED_GREEN_PIN, led);
    gpio_put(TRIGGER_PIN, led);
    put_pixel(urgb_u32(0x00, led ? 0x10 : 0x00, 0x00));
  }
}
