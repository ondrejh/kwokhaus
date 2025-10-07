#ifndef __BOARD_H__
#define __BOARD_H__

#define millis() (to_ms_since_boot(get_absolute_time()))

#define BTN_MIN_PULSE 10

typedef enum {
  BTNST_UNKNOWN,
  BTNST_UP,
  BTNST_DOWN,
  BTNST_PRESSED,
  BTNST_HOLD,
  BTNST_LONG_PRESSED,
} ButtonState;

#endif // __BOARD_H__
