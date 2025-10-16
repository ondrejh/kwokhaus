#ifndef __BOARD_H__
#define __BOARD_H__

#define millis() (to_ms_since_boot(get_absolute_time()))

#define BTN_MIN_PULSE 10
#define BTN_HOLD_PULSE 600
#define LOCK_FILTER_T 200

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

LockState lock_poll(uint32_t t);
ButtonState button_poll(uint32_t t);

#endif // __BOARD_H__
