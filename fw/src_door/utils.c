#include "includes.h"

/**
 * @brief Convert supply voltage to a PWM duty-cycle value.
 * @param voltage_mV Supply voltage in millivolts.
 * @return PWM value from 0 to PWM_PERIOD.
 */
uint32_t v2pwm(uint16_t voltage_mV) {
  if (voltage_mV == 0) return 0;

  if (voltage_mV <= VOLT_FULL_PWR) {
    return (uint32_t)PWM_PERIOD;
  }

  // Use 64-bit arithmetic to avoid overflow in the multiplication.
  uint64_t calc = (uint64_t)PWM_PERIOD * VOLT_FULL_PWR;
  return (uint32_t)(calc / voltage_mV);
}

/**
 * @brief Map linear light level to PWM with a perceptual (cubic) curve.
 *
 * The eye perceives brightness roughly as cube root of luminance, so a cubic
 * mapping makes a linear ramp of `level` look like an even fade.
 * @param level Linear level 0..PWM_PERIOD.
 * @param pwm_max Output PWM at full level (voltage compensated).
 * @return PWM value 0..pwm_max.
 */
uint32_t light_curve(uint32_t level, uint32_t pwm_max) {
  if (level >= PWM_PERIOD) return pwm_max;
  uint64_t x = (uint64_t)level * level / PWM_PERIOD; // level^2
  x = x * level / PWM_PERIOD;                         // level^3
  return (uint32_t)(x * pwm_max / PWM_PERIOD);
}
