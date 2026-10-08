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