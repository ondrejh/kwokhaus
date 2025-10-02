#include "includes.h"

#define millis() (to_ms_since_boot(get_absolute_time()))


uint8_t buf[SSD1306_BUF_LEN];

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

  SSD1306_init();
  struct render_area frame_area = {
    start_col : 0,
    end_col : SSD1306_WIDTH - 1,
    start_page : 0,
    end_page :SSD1306_NUM_PAGES - 1,
  };

  calc_render_area_buflen(&frame_area);
  render(&frame_area);

  for (int i=0; i<3; i++) {
    SSD1306_send_cmd(SSD1306_SET_ALL_ON);
    sleep_ms(500);
    SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON);
    sleep_ms(500);
  }
  
  char text[] = "Hello World";
  WriteString(10, 10, text);
  render(&frame_area);  
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
    if (led && ((now - tLed) >= 50)) {
      led = false;
      g = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 1000)) {
      led = true;
      g = 0x10;
      put_pixel(urgb_u32(r,g,b));
      printf("Hello World\r\n");
      
      uint8_t h,m,s;
      ds3231_get_time(&h, &m, &s);
      printf("%02d:%02d:%02d\n", h, m, s);
      
      tLed = now;
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

    //led = !led;
    //gpio_put(LED_GREEN_PIN, led);
    //gpio_put(TRIGGER_PIN, led);
    //put_pixel(urgb_u32(0x00, led ? 0x10 : 0x00, 0x00));
    //sleep_ms(1000);
  
    if ((now - tDisp) >= 10000) {
      //b = 0x10;
      //put_pixel(urgb_u32(r,g,b));

      tDisp = now;

      // display test
      /*u8g2_ClearBuffer(&u8g2);
      u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
      u8g2_DrawStr(&u8g2, 0, 15, "Hello World!");
      u8g2_SendBuffer(&u8g2);*/

      //i2c_bus_scan(RTC_I2C_PORT);
      //i2c_bus_scan(DISP_I2C_PORT);
      //printf("\n");

      //b = 0x00;
      //put_pixel(urgb_u32(r,g,b));
    }
  }
}
