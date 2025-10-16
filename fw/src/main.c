#include "includes.h"

void disp_splash(void) {
  int y = 27, x = 0;
  display_clear();
  display_set_font(&FreeMonoBold18pt7b);
  display_string(x, y, "Kwak", false);
  display_string(x + 35, y + FONT->yAdvance, "haus", false);
  display_show();
}

#define TMASK_NONE    0x00
#define TMASK_HOUR    0x01
#define TMASK_DOT     0x02
#define TMASK_MINUTE  0x04
#define TMASK_ALL     0x07

typedef struct {
  int x;
  int y;
} point_t;

const point_t timePos = {.x=10, .y=42,};

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

  //int w = 20 - 1; // test upper and lower space
  //DrawFrame(0, 0, 127, w);
  //DrawFrame(0, 63 - w, 127, 63);

  //char test[] = "AbCdEfGhIjKlMn"; // test upper and lower most text
  //display_set_font(&FreeMono9pt7b);
  //display_string(0, 9, test, false);
  //display_string(0, 60, test, false);
  
  // display time
  display_set_font(&FreeMonoBold18pt7b);
  display_string(timePos.x, timePos.y, text, false);
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

#define TIME_POLLING_PERIOD 1000 // ms

typedef enum {
  EVENT_NONE,
  EVENT_PRESS,
  EVENT_LONGPRESS,
  EVENT_TIME,
} event_t;

typedef struct {
  uint8_t h;
  uint8_t m;
  uint8_t s;
  uint8_t z;
} tim_t;

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  int32_t tLed = 0, tDisp = 0, tTrig = 0, tTim = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  tim_t tloc = {.z=2,};

  ds3231_get_time(&tloc.h, &tloc.m, &tloc.s);
  tloc.h = (tloc.h - tloc.z) % 24;

  tDisp = millis() + 3000; // some time for splash screen

  while (true) {
    int32_t now = millis();
    event_t event = EVENT_NONE;

    // collect button events
    if (event == EVENT_NONE) {
      ButtonState btn = button_poll(now);
      if (btn == BTNST_PRESSED)
        event = EVENT_PRESS;
      else if (btn == BTNST_LONG_PRESSED)
        event = EVENT_LONGPRESS;
    }

    // test time update event
    if ((event == EVENT_NONE) && ((now - tTim) >= TIME_POLLING_PERIOD)) {
      uint8_t h, m;
      ds3231_get_time(&h, &m, &tloc.s);
      h = (h - tloc.z) % 24;
      if ((m != tloc.m) || (h != tloc.h))
        event = EVENT_TIME;
      tloc.h = h;
      tloc.m = m;
      tTim = now;
    }

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
      tLed = now;
    }

    // display
    if ((now - tDisp) >= 125) {
      static uint8_t cnt = 0;
      static uint8_t h, m;
      switch (cnt & 0x07) {
        case 0:
          h = tloc.h;
          m = tloc.m;
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

    // event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_TIME:
        printf("Time %02d:%02d\n", tloc.h, tloc.m);
        break;
      case EVENT_NONE:
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
