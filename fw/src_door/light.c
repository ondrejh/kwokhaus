#include "includes.h"

// light output: dimming ramp and PWM output
// target state is set by main logic (light_set), ramp runs in the io task

static volatile bool light_on = false;
static volatile uint32_t light_dimming = LIGHT_DIMMING;

void light_init(void) {
  gpio_set_function(LIGHT_PIN, GPIO_FUNC_PWM);
  uint slice = pwm_gpio_to_slice_num(LIGHT_PIN);
  pwm_set_wrap(slice, PWM_PERIOD);
  pwm_set_gpio_level(LIGHT_PIN, 0);
  pwm_set_enabled(slice, true);
}

// set target light state and dimming speed (ramp step per update period)
void light_set(bool on, uint32_t dimming) {
  light_dimming = dimming;
  light_on = on;
}

bool light_is_on(void) {
  return light_on;
}

// ramp light level towards target, pwm_max is voltage compensated full level
void light_update(uint32_t now, uint32_t pwm_max) {
  static uint32_t tLight = 0;
  static uint32_t level = 0; // linear ramp 0..PWM_PERIOD, mapped by light_curve()

  if ((now - tLight) <= LIGHT_UPDATE_PERIOD)
    return;
  tLight = now;

  uint32_t dimming = light_dimming;
  if (light_on) {
    if (level < PWM_PERIOD) {
      level += dimming;
      if (level > PWM_PERIOD)
        level = PWM_PERIOD;
    }
  }
  else {
    if (level > dimming)
      level -= dimming;
    else
      level = 0;
  }
  pwm_set_gpio_level(LIGHT_PIN, light_curve(level, pwm_max));
}
