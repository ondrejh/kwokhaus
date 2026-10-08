#include "includes.h"

static const uint16_t adc_vref = 3300; // 3.3V

void adc_module_init(void) {
  adc_init();
  adc_gpio_init(SENSE_VIN_PIN);
  adc_gpio_init(SENSE_L_PIN);
  adc_gpio_init(SENSE_R_PIN);
}

uint16_t adc2u(int32_t adc) {
  uint32_t res = adc * adc_vref * 11 / (4096 * ADC_OVERSAMPLE);
  return res;
}

bool adc_poll(uint32_t now, int32_t *adc) {
  static uint32_t tAdc = 0;
  static int cnt = 0;
  static int32_t a[3] = {0, 0, 0};
  if ((now - tAdc) >= ADC_POLL_PERIOD) {
    tAdc = now;
    adc_select_input(SENSE_VIN_ADC);
    a[0] += adc_read();
    adc_select_input(SENSE_L_ADC);
    a[1] += adc_read();
    adc_select_input(SENSE_R_ADC);
    a[2] += adc_read();
    cnt++;
    if (cnt >= ADC_OVERSAMPLE) {
      for (int i = 0; i < 3; i++) {
        adc[i] = a[i];
        a[i] = 0;
      }
      cnt = 0;
      return true;
    }
  }
  return false;
}