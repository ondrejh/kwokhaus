#include "includes.h"

// io task: hardware outputs driven periodically
// - motor (ADC, driver, current monitoring)
// - light dimming
// - RGB status LED

void io_init(void) {
  motor_init();
  light_init();
  ws2812_init(RGB_LED_PIN);
}

// status LED: blinking green/red while moving up/down,
// steady green/red in the end position, blue when light is on
static void led_update(uint32_t now) {
  static uint32_t tLed = 0;
  static uint8_t r = 0, g = 0;

  if ((now - tLed) <= LED_UPDATE_PERIOD)
    return;
  tLed = now;

  uint8_t status = motor_get_status();
  if (status & (MOTOR_GO_UP | MOTOR_FORCE_UP)) {
    g = (g == 0x10) ? 0 : 0x10;
    r = 0;
  }
  else if (status & (MOTOR_GO_DOWN | MOTOR_FORCE_DOWN)) {
    r = (r == 0x10) ? 0 : 0x10;
    g = 0;
  }
  else if (status & MOTOR_IS_END) {
    g = (status & MOTOR_IS_UP) ? 0x10 : 0;
    r = (status & MOTOR_IS_DOWN) ? 0x10 : 0;
  }
  else {
    r = g = 0;
  }
  uint8_t b = light_is_on() ? 0x08 : 0;
  put_pixel(urgb_u32(r, g, b));
}

void io_task(void *param) {
  (void)param;

  TickType_t wake = xTaskGetTickCount();

  while (true) {
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(IO_TASK_PERIOD_MS));
    uint32_t now = millis();

    motor_update(now);
    light_update(now, motor_get_pwm());
    led_update(now);
  }
}
