#include "includes.h"


void init(void) {
  // Initialize outputs
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_pulls(BUTTON_PIN, true, false);
  gpio_set_dir(LOCK_PIN, GPIO_IN);
  gpio_set_pulls(LOCK_PIN, true, false);

  // Initialize communication
  comm_init();

  // Initialize onboard NeoPixel
  ws2812_init(16);
}

#define COMM_BUFLEN 128
uint8_t comm_buff[COMM_BUFLEN];

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  int32_t tLed = 0, tTrig = 0, tTim = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  while (true) {
    int32_t now = millis();
    bool trig_now = false;

    // grab events
    event_t event = get_input_event(now); // input events

    // receive comm
    int comrx = comm_poll(now, 100, comm_buff, COMM_BUFLEN);
    if (comrx) { // echo test
      comrx = strip(comm_buff, comrx);
      if (comrx) {
        comm_buff[comrx] = '\0';
        printf("RX: %s\n", comm_buff);

        comrx = comm_parse(comm_buff, comrx, COMM_BUFLEN, &event);
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
    }

    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 2000)) {
      /*if (!comm_tx_busy()) {
        comm_write("Hello UART\r\n", 12);
      }*/
      led = true;
      b = 0x10;
      put_pixel(urgb_u32(r,g,b));
      tLed = now;
    }

    // event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_LOCK:
      case EVENT_UNLOCK:
        printf("Lock %s\n", (event==EVENT_LOCK)?"locked":"unlocked");
        comrx = sprint_status(comm_buff, COMM_BUFLEN);
        comm_write(comm_buff, comrx);
        break;
      case EVENT_CMD_UNLOCK:
        printf("Unlock command received\n");
        trig_now = true;
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
    if (!trig && trig_now) {
      tTrig = now;
      trig = true;
      gpio_put(TRIGGER_PIN, trig);
      r = 0x10;
      put_pixel(urgb_u32(r,g,b));
    }

    gpio_put(LED_GREEN_PIN, comm_tx_busy());
  }
}
