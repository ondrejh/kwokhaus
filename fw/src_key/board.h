#ifndef __BOARD_H__
#define __BOARD_H__

#define BUTTON_PIN 11
#define RGB_LED_PIN 10

#define LED_INIT() {}
#define LED_ON() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,1)
#define LED_OFF() cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,0)

#define millis() (to_ms_since_boot(get_absolute_time()))

#define BTN_MIN_PULSE 10
#define BTN_HOLD_PULSE 600

typedef enum {
  LOCK_UNKNOWN,
  LOCK_LOCKED,
  LOCK_UNLOCKED,
} LockState;

typedef enum {
  BTNST_UNKNOWN,
  BTNST_UP,
  BTNST_DOWN,
  BTNST_PRESSED,
  BTNST_HOLD,
  BTNST_LONG_PRESSED,
} ButtonState;

ButtonState button_poll(uint32_t t);

#endif // __BOARD_H__
