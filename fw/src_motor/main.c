#include "includes.h"

void init(void) {
  // Initialize outputs
  gpio_init(ENABLE_R_PIN);
  gpio_set_dir(ENABLE_R_PIN, GPIO_OUT);
  gpio_init(ENABLE_L_PIN);
  gpio_set_dir(ENABLE_L_PIN, GPIO_OUT);

  gpio_init(PWM_R_PIN);
  gpio_set_dir(PWM_R_PIN, GPIO_OUT);
  gpio_init(PWM_L_PIN);
  gpio_set_dir(PWM_L_PIN, GPIO_OUT);

  // Initialize nvdata (load configuration)
  load_config(&config);

  // Initialize communication
  //comm_init();

  // Initialize onboard NeoPixel
  ws2812_init(16);
}

#define COMM_BUFLEN 128
uint8_t comm_buff[COMM_BUFLEN];

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  int32_t tLed = 0, tDisp = 0, tTrig = 0, tTim = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  btn_ctx_t btnL, btnR;
  button_init(&btnL, BUTTON_L_PIN);
  button_init(&btnR, BUTTON_R_PIN);

  tDisp = millis();

  while (true) {
    int32_t now = millis();

    button_poll(&btnL, now);
    button_poll(&btnR, now);

    if (btnL.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      gpio_put(PWM_R_PIN, false);
      gpio_put(PWM_L_PIN, true);
      if ((r == 0) || (g != 0)) {
        g = 0;
        r = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else if (btnR.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, true);
      if ((g == 0) || (r != 0)) {
        r = 0;
        g = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else {
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, false);
      gpio_put(ENABLE_L_PIN, false);
      gpio_put(ENABLE_R_PIN, false);
      if ((r != 0) || (g != 0)) {
        r = g = 0;
        put_pixel(urgb_u32(r,g,b));
      }
    }

    //event_t event = get_input_event(now);

    // receive comm
    /*int comrx = comm_poll(now, 100, comm_buff, COMM_BUFLEN);
    if (comrx) { // echo test
      comrx = strip(comm_buff, comrx);
      if (comrx) {
        comm_buff[comrx] = '\0';
        printf("RX: %s\n", comm_buff);

        comrx = comm_parse(comm_buff, comrx, COMM_BUFLEN);
        if (comrx) {
          comm_buff[comrx] = '\0';
          if (!comm_tx_busy()) {
            comm_write(comm_buff, comrx);
            printf("TX: %s\n", comm_buff);
          }
          else
            printf("TX BUSY\n");
        }
      }
    }*/

    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 1000)) {
      //if (!comm_tx_busy()) {
      //  comm_write("Hello UART\r\n", 12);
      //}
      led = true;
      b = 0x10;
      put_pixel(urgb_u32(r,g,b));
      tLed = now;
    }

    /*// event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_NONE:
      default:
        break;
    }*/
  }
}
