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

  uint8_t motor_status = 0;
  uint32_t motor_running_t = 0;
  uint32_t motor_current_t = 0;

  while (true) {
    int32_t now = millis();

    button_poll(&btnL, now);
    button_poll(&btnR, now);

    // start motor up/down
    if (btnL.st == BTNST_PRESSED) {
      if (motor_status & MOTOR_RUNNING) {
        printf("MOTOR STOP\n");
        motor_status &= ~MOTOR_RUNNING;
      }
      else {
        printf("MOTOR UP\n");
        motor_status |= MOTOR_GO_UP;
        motor_running_t = now;
      }
    }
    else if (btnR.st == BTNST_PRESSED) {
      if (motor_status & MOTOR_RUNNING) {
        printf("MOTOR STOP\n");
        motor_status &= ~MOTOR_RUNNING;
      }
      else {
        printf("MOTOR DOWN\n");
        motor_status |= MOTOR_GO_DOWN;
        motor_running_t = now;
      }
    }

    if (motor_status & MOTOR_RUNNING) {
      if ((now - motor_running_t) > MOTOR_SAFETY_TIMEOUT) { // safety timeout
        printf("MOTOR TIMEOUT\n");
        motor_status &= ~MOTOR_RUNNING;
      }
    }

    // force motor up/down
    if (btnL.st == BTNST_HOLD) {
      motor_status |= MOTOR_FORCE_UP;
      motor_status &= ~(MOTOR_GO_DOWN | MOTOR_FORCE_DOWN | MOTOR_GO_UP);
    }
    else if (btnR.st == BTNST_HOLD) {
      motor_status |= MOTOR_FORCE_DOWN;
      motor_status &= ~(MOTOR_GO_UP | MOTOR_FORCE_UP | MOTOR_GO_DOWN);
    }
    else {
      motor_status &= ~(MOTOR_FORCE_UP | MOTOR_FORCE_DOWN);
    }

    if (motor_status & (MOTOR_GO_UP | MOTOR_FORCE_UP)) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      pwm_set_gpio_level(PWM_R_PIN, pwm);
      pwm_set_gpio_level(PWM_L_PIN, 0);
    }
    else if (motor_status & (MOTOR_GO_DOWN | MOTOR_FORCE_DOWN)) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      pwm_set_gpio_level(PWM_R_PIN, 0);
      pwm_set_gpio_level(PWM_L_PIN, pwm);
    }
    else {
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, false);
      pwm_set_gpio_level(PWM_R_PIN, 0);
      pwm_set_gpio_level(PWM_L_PIN, 0);
    }

    // read ADC and calculate voltage and PWM value
    if (adc_poll(now, adc)) {
      voltage = adc2u(adc[0]);
      pwm = v2pwm(voltage);
    }

#ifdef DEBUG
    if ((now - tAdc) > 200) {
      tAdc = now;
      printf("%04X %04X %04X\n", adc[0], adc[1], adc[2]);
      printf("%0.01fV, %d\n", (float)voltage/1000.0, pwm);
      printf("Motor status: %x\n", motor_status);
    }
#endif

    // motor current monitoring
    if (motor_status & MOTOR_RUNNING) {
      motor_status &= ~MOTOR_IS_END;
      if ((adc[1] > CURRENT_MIN) || (adc[2] > CURRENT_MIN)) {
        motor_current_t = now;
      }
      else if ((now - motor_current_t) > MOTOR_CURRENT_TIMEOUT) {
        if (motor_status & MOTOR_GO_UP) {
          motor_status |= MOTOR_IS_UP;
          printf("MOTOR IS UP\n");
        }
        else if (motor_status & MOTOR_GO_DOWN) {
          motor_status |= MOTOR_IS_DOWN;
          printf("MOTOR IS DOWN\n");
        }
        motor_status &= ~MOTOR_RUNNING;
      }
    } else {
      motor_current_t = now;
    }

    // led status
    if ((now - tLed) > 250) {
      tLed = now;
      if (motor_status & (MOTOR_GO_UP | MOTOR_FORCE_UP)) {
        g = (g == 0x10) ? 0 : 0x10;
        r = 0;
      }
      else if (motor_status & (MOTOR_GO_DOWN | MOTOR_FORCE_DOWN)) {
        r = (r == 0x10) ? 0 : 0x10;
        g = 0;
      }
      else if (motor_status & MOTOR_IS_END) {
        g = (motor_status & MOTOR_IS_UP) ? 0x10 : 0;
        r = (motor_status & MOTOR_IS_DOWN) ? 0x10 : 0;
      }
      else {
        r = g = 0;
      }
      put_pixel(urgb_u32(r,g,b));
    }
  }
}
