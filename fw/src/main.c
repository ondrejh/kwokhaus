#include "includes.h"

void disp_splash(void) {
  int y = 27, x = 0;
  display_clear();
  display_set_font(&FreeMonoBold18pt7b);
  display_string(x, y, "Kwak");
  display_string(x + 35, y + FONT->yAdvance, "haus");
  display_show();
}

#define TMASK_NONE    0x00
#define TMASK_HOUR    0x01
#define TMASK_DOT     0x02
#define TMASK_MINUTE  0x04
#define TMASK_ALL     0x07

void disp_time(uint8_t hour, uint8_t minute, uint8_t mask) {
  char text[6] = "     ";

  if ((mask & TMASK_HOUR) == 0) {
    text[0] = (char)(hour / 10 % 10) + '0';
    text[1] = (char)(hour % 10) + '0';
  }
  if ((mask & TMASK_DOT) == 0)
    text[2] = ':';
  if ((mask & TMASK_MINUTE) == 0) { 
    text[3] = (char)(minute / 10 % 10) + '0';
    text[4] = (char)(minute % 10) + '0';
  }

  display_clear();
  display_set_font(&FreeMonoBold18pt7b);
  display_string(10, 44, text);
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

  uint8_t h, m, s;
  ds3231_get_time(&h, &m, &s);

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
      
      printf("%02d:%02d:%02d\n", h, m, s);
      
      tLed = now;
    }

    // display
    if ((now - tDisp) >= 125) {
      static uint8_t cnt = 0;
      switch (cnt & 0x07) {
        case 0:
          ds3231_get_time(&h, &m, &s);
          disp_time(h, m, TMASK_NONE);
          break;
        case 4:
          disp_time(h, m, TMASK_DOT);
          break;
        default:
          break;
      }
      cnt ++;
      tDisp = now;
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

    /*// display (so far)
    if ((now - tDisp) >= 10000) {
      tDisp = now;
      disp_time();
    }*/
  }
}
