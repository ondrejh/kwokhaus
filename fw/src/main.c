#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "ws2812.pio.h"
#include "ws2812.h"

#include "u8g2.h"

#define LED_GREEN_PIN 12
#define BUTTON_PIN 13
#define TRIGGER_PIN 14
#define SENSE_ADC_PIN 26

#define millis() (to_ms_since_boot(get_absolute_time()))

#define DISPLAY

#ifdef DISPLAY

// Display I2C pins and instance
#define DISP_I2C_SDA_PIN 4
#define DISP_I2C_SCL_PIN 5
#define DISP_I2C_PORT i2c0

// U8g2 structure
u8g2_t u8g2;

// Send function for U8g2
uint8_t u8x8_byte_pico_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  switch(msg) {
    case U8X8_MSG_BYTE_INIT:
    case U8X8_MSG_BYTE_START_TRANSFER:
    case U8X8_MSG_BYTE_END_TRANSFER:
      return 1;
    case U8X8_MSG_BYTE_SEND:
      i2c_write_blocking(DISP_I2C_PORT, u8x8_GetI2CAddress(u8x8) >> 1, arg_ptr, arg_int, false);
      return 1;
    default:
      return 0;
  }
}

// Delay handler for U8g2
uint8_t u8x8_gpio_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  switch(msg) {
    case U8X8_MSG_DELAY_MILLI:
      sleep_ms(arg_int);
      return 1;
    default:
      return 0;
  }
}
#endif // DISPLAY

void init(void) {
#ifdef DISPLAY
  // Initialize display I2C
  i2c_init(DISP_I2C_PORT, 400 * 1000);

  // Initialize display (128x64)
  u8g2_Setup_ssd1306_i2c_128x64_noname_f(
    &u8g2,
    U8G2_R0,
    u8x8_byte_pico_i2c,
    u8x8_gpio_delay_pico
  );
  u8g2_SetI2CAddress(&u8g2, 0x78); // 0x78 = 0x3C << 1
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
#endif // DISPLAY

  // Initialize outputs
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);

  // Initialize onboard NeoPixel
  ws2812_init(16);
}

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false;
  uint32_t tLed = 0, tDisp = 0;

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
  
#ifdef DISPLAY
    if ((now - tDisp) >= 1000) {
      tDisp = now;
      // display test
      u8g2_ClearBuffer(&u8g2);
      u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
      u8g2_DrawStr(&u8g2, 0, 24, "Hello World!");
      u8g2_SendBuffer(&u8g2);
    }
#endif // DISPLAY
  }
}
