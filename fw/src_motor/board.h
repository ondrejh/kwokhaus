#ifndef __BOARD_H__
#define __BOARD_H__

#define millis() (to_ms_since_boot(get_absolute_time()))

#define BTN_MIN_PULSE 10
#define BTN_HOLD_PULSE 300

typedef enum {
  BTNST_UNKNOWN,
  BTNST_UP,
  BTNST_DOWN,
  BTNST_PRESSED,
  BTNST_HOLD,
  BTNST_LONG_PRESSED,
} ButtonState;

typedef struct btn_ctx_t {
  uint32_t t;
  uint32_t cnt1, cnt2;
  int pin;
  ButtonState st;
} btn_ctx_t;

void button_init(btn_ctx_t *ctx, int pin);
ButtonState button_poll(btn_ctx_t *ctx, uint32_t now);

#endif // __BOARD_H__
