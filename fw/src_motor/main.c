#include "includes.h"

const uint16_t adc_vref = 3300; // 3.3V

uint16_t adc2u(uint16_t adc) {
  uint32_t res = adc * adc_vref * 11 / (4096 * ADC_OVERSAMPLE) ;
  return res;
}

bool adc_poll(uint32_t now, uint16_t *adc) {
  static uint32_t tAdc = 0;
  static int cnt = 0;
  static uint16_t a[3] = {0,0,0};
  if ((now - tAdc) >= ADC_POLL_PERIOD) {
    tAdc = now;
    adc_select_input(SENSE_VIN_ADC);
    a[0] += adc_read();
    adc_select_input(SENSE_L_ADC);
    a[1] += adc_read();
    adc_select_input(SENSE_R_ADC);
    a[2] += adc_read();
    cnt ++;
    if (cnt >= ADC_OVERSAMPLE) {
      for (int i=0; i<3; i++) {
        adc[i] = a[i];
        a[i] = 0;
      }
      cnt = 0;
      return true;
    }
  }
  return false;
}

uint32_t v2pwm(uint16_t v) {
  // 1. osetreni 0
  if (v == 0) return 0;

  // 2. pokud je napeti nizsi nebo rovno limitu, jedeme na 100 %
  if (v <= VOLT_FULL_PWR) {
    return (uint32_t)PWM_PERIOD;
  }

  // 3. vypocet pro vyssi napeti: PWM = PERIOD * (LIMIT / NAPETI)
  // pouzivame uint64_t pro, aby nedoslo k preteceni
  uint64_t calc = (uint64_t)PWM_PERIOD * VOLT_FULL_PWR;
  return (uint32_t)(calc / v);
}

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

  // initialize PWM
  // 1. nastaveni pwm pinu
  gpio_set_function(PWM_L_PIN, GPIO_FUNC_PWM);
  gpio_set_function(PWM_R_PIN, GPIO_FUNC_PWM);

  // 2. zjisti bloky (slices) pwm
  uint sliceL = pwm_gpio_to_slice_num(PWM_L_PIN);
  uint sliceR = pwm_gpio_to_slice_num(PWM_R_PIN);

  // 3. nastaveni periody
  pwm_set_wrap(sliceL, PWM_PERIOD);
  pwm_set_wrap(sliceR, PWM_PERIOD);

  // 4. nastaveni stridy (Duty Cycle) ~ 0
  pwm_set_gpio_level(PWM_L_PIN, 0);
  pwm_set_gpio_level(PWM_R_PIN, 0);

  // 5. spusteni PWM bloku
  pwm_set_enabled(sliceL, true);
  pwm_set_enabled(sliceR, true);

  // initialize ADC
  adc_init();
  adc_gpio_init(SENSE_VIN_PIN);
  adc_gpio_init(SENSE_L_PIN);
  adc_gpio_init(SENSE_R_PIN);

  // Initialize onboard NeoPixel
  ws2812_init(RGB_LED_PIN);
}

#define COMM_BUFLEN 128
uint8_t comm_buff[COMM_BUFLEN];

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  int32_t tLed = 0, tDisp = 0, tTrig = 0, tTim = 0, tAdc = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  btn_ctx_t btnL, btnR;
  button_init(&btnL, BUTTON_L_PIN);
  button_init(&btnR, BUTTON_R_PIN);

  tDisp = millis();

  uint32_t cnt = 0;
  uint16_t adc[3];

  uint32_t pwm = 0;
  uint16_t voltage = 0;

  while (true) {
    int32_t now = millis();

    button_poll(&btnL, now);
    button_poll(&btnR, now);

    if (btnL.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      pwm_set_gpio_level(PWM_R_PIN, 0);
      pwm_set_gpio_level(PWM_L_PIN, pwm);
      if ((r == 0) || (g != 0)) {
        g = 0;
        r = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else if (btnR.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      pwm_set_gpio_level(PWM_R_PIN, pwm);
      pwm_set_gpio_level(PWM_L_PIN, 0);
      if ((g == 0) || (r != 0)) {
        r = 0;
        g = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else {
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, false);
      pwm_set_gpio_level(PWM_R_PIN, 0);
      pwm_set_gpio_level(PWM_L_PIN, 0);
      if ((r != 0) || (g != 0)) {
        r = g = 0;
        put_pixel(urgb_u32(r,g,b));
      }
    }

    if (adc_poll(now, adc)) {
      voltage = adc2u(adc[0]);
      pwm = v2pwm(voltage);
    }

    if ((now - tAdc) > 200) {
      tAdc = now;
      printf("%04X %04X %04X\n", adc[0], adc[1], adc[2]);
      printf("%0.01fV, %d\n", (float)voltage/1000.0, pwm);
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
      //printf("%d %d %d %d %d\n", cnt, adc[0], adc[1], adc[2], adc2u(adc[0]));
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
