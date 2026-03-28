#include "includes.h"

#define USE_PWM_OUT

const uint16_t adc_vref = 3300; // 3.3V

#define VIN_CORR_PERCENT 86

uint16_t adc2u(uint16_t adc) {
  uint16_t res = (uint32_t)(((uint32_t)adc * adc_vref * 11) / ((uint32_t)4096 * ADC_OVERSAMPLE) * VIN_CORR_PERCENT / 100);
  return (uint16_t)res;
}

uint32_t v2pwm(uint16_t v) {
  if (v == 0) return 0;
  if (v <= VOLT_FULL_PWR) {
    return (uint32_t)PWM_PERIOD;
  }
  uint64_t calc = (uint64_t)PWM_PERIOD * VOLT_FULL_PWR;
  return (uint32_t)(calc / v);
}

bool adc_poll(uint32_t now, uint16_t *adc) {
  static uint32_t tAdc = 0;
  static int cnt = 0;
  static uint16_t a = 0;
  if ((now - tAdc) >= ADC_POLL_PERIOD) {
    tAdc = now;
    adc_select_input(SENSE_VIN_ADC);
    a += adc_read();
    cnt ++;
    if (cnt >= ADC_OVERSAMPLE) {
      *adc = a;
      a = 0;
      cnt = 0;
      return true;
    }
  }
  return false;
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
  gpio_set_dir(LOCK_PIN, GPIO_IN);
  gpio_set_pulls(LOCK_PIN, true, false);

#ifdef USE_PWM_OUT
  // Initialize PWM
  gpio_set_function(TRIGGER_PIN, GPIO_FUNC_PWM);
  uint slTrig = pwm_gpio_to_slice_num(TRIGGER_PIN);
  pwm_set_wrap(slTrig, PWM_PERIOD);
  pwm_set_gpio_level(TRIGGER_PIN, 0);
  pwm_set_enabled(slTrig, true);
#endif

    // initialize ADC
  adc_init();
  adc_gpio_init(SENSE_VIN_PIN);

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

  bool led = false;
  int32_t tLed = 0;

  bool trig = false;
  int32_t tTrig = 0, tLastTx = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00; 

  int16_t adcVin = 0;
  int32_t tAdcNot = 0;

  uint32_t pwm = 0;
  uint16_t voltage = 0;

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
            tLastTx = now;
            comm_write(comm_buff, comrx);
            printf("TX: %s\n", comm_buff);
          }
          else
            printf("TX BUSY\n");
        }
      }
    }

    if ((!comm_tx_busy()) && ((now - tLastTx) > STATUS_REPEAT_PERIOD)) {
      tLastTx = now;
      comrx = sprint_status(comm_buff, COMM_BUFLEN);
      comm_write(comm_buff, comrx);
      printf("TX: %s\n", comm_buff);
    }

  #ifdef LIFE_LED
    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 2000)) {
      led = true;
      b = 0x10;
      put_pixel(urgb_u32(r,g,b));
      tLed = now;
    }
  #endif

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
        tLastTx = now;
        break;
      case EVENT_CMD_UNLOCK:
        printf("Unlock command received\n");
        trig_now = true;
        break;
      case EVENT_NONE:
      default:
        break;
    }

    // measure input voltage, get current pwm output (12V out)
    if (adc_poll(now, &adcVin)) {
      voltage = adc2u(adcVin);
      pwm = v2pwm(voltage);
#ifdef USE_PWM_OUT
      pwm_set_gpio_level(TRIGGER_PIN, trig ? pwm : 0);
#endif
    }
    /*// print it (debug)
    if ((now - tAdcNot) > 2000) {
      tAdcNot = now;
      printf("%0.01fV %d/%d DC\n", (float)voltage/1000.0, PWM_PERIOD, pwm);
    }*/

    // trigger
    if (trig && ((now - tTrig) >= 500)) {
      trig = false;
#ifndef USE_PWM_OUT
      gpio_put(TRIGGER_PIN, trig);
#endif
      r = 0;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!trig && trig_now) {
      tTrig = now;
      trig = true;
#ifndef USE_PWM_OUT
      gpio_put(TRIGGER_PIN, trig);
#endif
      r = 0x80;
      put_pixel(urgb_u32(r,g,b));
    }

    gpio_put(LED_GREEN_PIN, comm_tx_busy());
  }
}
