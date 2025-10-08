#include "includes.h"

void disp_splash(void) {
  int y = 27, x = 0;
  display_clear();
  display_set_font(&FreeMonoBold18pt7b);
  display_string(x, y, "Kwak");
  display_string(x + 35, y + FONT->yAdvance, "haus");
  display_show();
}

void disp_time(void) {
  display_clear();
  display_set_font(&FreeMonoBold18pt7b);
  display_string(10, 44, "00:00");
  display_show();
}

void init(void) {
  // Initialize outputs
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_pulls(BUTTON_PIN, true, false);

  // Initialize display I2C
  i2c_init(DISP_I2C_PORT, 400 * 1000);
  gpio_set_function(DISP_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(DISP_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(DISP_I2C_SDA_PIN);
  gpio_pull_up(DISP_I2C_SCL_PIN);

  // Initialize RTC I2C
  i2c_init(RTC_I2C_PORT, 400 * 1000);
  gpio_set_function(RTC_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(RTC_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(RTC_I2C_SDA_PIN);
  gpio_pull_up(RTC_I2C_SCL_PIN);

  // Initialize onboard NeoPixel
  ws2812_init(16);

  // Initialize display
  SSD1306_init();
  disp_splash();
}

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  uint32_t tLed = 0, tDisp = 0, tTrig = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  sleep_ms(100);

  while (true) {
    uint32_t now = millis();

    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      g = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 2000)) {
      led = true;
      g = 0x10;
      put_pixel(urgb_u32(r,g,b));
      printf("Hello World\r\n");
      
      uint8_t h,m,s;
      ds3231_get_time(&h, &m, &s);
      printf("%02d:%02d:%02d\n", h, m, s);
      
      tLed = now;
    }

    // button state test
    ButtonState btn = button_poll(now);
    switch (btn) {
      case BTNST_PRESSED:
        printf("Button pressed\n");
        break;
      case BTNST_LONG_PRESSED:
        printf("button long pressed\n");
        break;
      default:
        break;
    }

    // trigger
    if (trig && ((now - tTrig) >= 500)) {
      trig = false;
      gpio_put(TRIGGER_PIN, trig);
      r = 0;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!trig && (gpio_get(BUTTON_PIN) == 0)) {
      tTrig = now;
      trig = true;
      gpio_put(TRIGGER_PIN, trig);
      r = 0x10;
      put_pixel(urgb_u32(r,g,b));
    }

    // display (so far)
    if ((now - tDisp) >= 10000) {
      tDisp = now;
      disp_time();
    }
  }
}
