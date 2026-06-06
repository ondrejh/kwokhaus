#include "includes.h"

#define USE_PWM_OUT

extern LockState lock;
bool light = false;
extern PirState pir;

const uint16_t adc_vref = 3300; // 3.3V

uint16_t adc2u(uint16_t adc) {
  uint16_t res = (uint32_t)(((uint32_t)adc * adc_vref * 11) / ((uint32_t)4096 * ADC_OVERSAMPLE));
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
  // LED
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  // Unlock trigger
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
  // Button
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_pulls(BUTTON_PIN, true, false);
  // Lock state input
  gpio_init(LOCK_PIN);
  gpio_set_dir(LOCK_PIN, GPIO_IN);
  gpio_set_pulls(LOCK_PIN, true, false);
  // PIR sensor input
  gpio_init(PIR_INPUT_PIN);
  gpio_set_dir(PIR_INPUT_PIN, GPIO_IN);
  gpio_set_pulls(PIR_INPUT_PIN, true, false);

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
  led_rgb_onboard_init(16);
  led_onboard(urgb_u32(0, 0, 0));

  // Initialize RGB Light
  rgb_light_init(3);
  rgb_light(urgb_u32(0, 0, 0));

  // BME280 temperature, pressure, humidity sensor init
  if (!bme280_init()) {
    printf("BME280 not connected!\n");
  }
}

#define COMM_BUFLEN 256
uint8_t comm_buff[COMM_BUFLEN];

uint32_t light_power_to_rgb(uint16_t pwr) {
  if (pwr <= 0)
    return urgb_u32(0, 0, 0);
  if (pwr >= LIGHT_PWR_MAX)
    return urgb_u32(255, 255, 255);
  uint8_t p = pwr * 255 / LIGHT_PWR_MAX;
  return urgb_u32(p, p, p);
}

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

  //bool light = false;
  uint16_t lightPwr = 0;
  uint32_t tLight = 0;
  uint32_t lightChng = LIGHT_CHANGE_SLOW;
  uint32_t tLightOff = 0;
  uint32_t lightOffTout = 0;

  uint32_t tBme280 = 0;

  bool kick_status = false;

  while (true) {
    int32_t now = millis();
    bool trig_now = false;

    // grab events
    event_t event = get_input_event(now); // input events

    // receive comm
    int comrx = comm_poll(now, COMM_TIMEOUT, comm_buff, COMM_BUFLEN);
    if (comrx) { // echo test
      comrx = strip(comm_buff, comrx);
      if (comrx) {
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

    if ((!comm_tx_busy()) && ( \
      ((now - tLastTx) > STATUS_REPEAT_PERIOD) || \
      (kick_status && ((now - tLastTx) > STATUS_REPEAT_MIN_PERIOD)) \
    )) {
      tLastTx = now;
      kick_status = false;
      comrx = sprint_status(comm_buff, COMM_BUFLEN);
      comm_write(comm_buff, comrx);
      printf("TX: %s\n", comm_buff);
    }

  #ifdef LIFE_LED
    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      led_onboard(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 2000)) {
      led = true;
      b = 0x10;
      led_onboard(urgb_u32(r,g,b));
      tLed = now;
    }
  #endif

    // event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        light = !light;
        lightChng = LIGHT_CHANGE_FAST;
        lightOffTout = light ? LIGHT_OFF_TIMEOUT : 0;
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_LOCK:
        lightOffTout = LIGHT_OFF_TIMEOUT;
        lightChng = LIGHT_CHANGE_SLOW;
      case EVENT_UNLOCK:
        printf("Lock %s\n", (event==EVENT_LOCK)?"locked":"unlocked");
        kick_status = true;
        break;
      case EVENT_CMD_UNLOCK:
        printf("Unlock command received\n");
        trig_now = true;
        break;
      case EVENT_CMD_LIGHT:
        printf("Light ON command received\n");
        light = true;
        kick_status = true;
        if (lock==LOCK_LOCKED) {
          lightOffTout = LIGHT_OFF_TIMEOUT;
        }
        break;
      case EVENT_FREE:
        printf("Kwakhaus is free\n");
        gpio_put(LED_GREEN_PIN, false);
        kick_status = true;
        break;
      case EVENT_OCUPY:
        printf("Kwakhaus ocupied\n");
        gpio_put(LED_GREEN_PIN, true);
        kick_status = true;
        break;
      case EVENT_NONE:
      default:
        break;
    }

    // look for changes
    /*static LockState last_lock = LOCK_UNKNOWN;
    static bool last_light = false;
    static PirState last_pir = PIRST_UNKNOWN;
    if (((now - tLastTx) > STATUS_REPEAT_MIN_PERIOD) && ((lock != last_lock) || (pir != last_pir) || (light != last_light))) {
      kick_status = true;
      last_lock = lock;
      last_light = light;
      last_pir = pir;
    }*/

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
      led_onboard(urgb_u32(r,g,b));
    }
    if (!trig && trig_now) {
      tTrig = now;
      trig = true;
#ifndef USE_PWM_OUT
      gpio_put(TRIGGER_PIN, trig);
#endif
      r = 0x80;
      led_onboard(urgb_u32(r,g,b));
    }

    // light off timeout
    if (light & (lightOffTout != 0)) {
      if ((now - tLightOff) >= (lightOffTout * 1000)) {
        light = false;
        kick_status = true;
        lightOffTout = 0;
      }
    }
    else {
      tLightOff = now;
    }

    // light shade in/out
    if ((light && (lightPwr<LIGHT_PWR_MAX)) || (!light && (lightPwr > 0))) {
      if ((now - tLight) >= lightChng) {
        tLight = now;
        lightPwr += light ? 1 : -1;
        rgb_light(light_power_to_rgb(lightPwr));
      }
    } 
    else {
      tLight = now;
    }

    // environment polling
    if (env_poll(now)) {
      printf("Envi: %.1fC, %.2fkPa, %.0f%%\n", env_temp / 100.f, env_press / 1000.f, env_humi / 1024.f);
    }

    //gpio_put(LED_GREEN_PIN, comm_tx_busy());
  }
}
