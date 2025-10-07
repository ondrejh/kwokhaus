#include "includes.h"


ButtonState button_poll(uint32_t now) {
  static uint32_t t = 0;
  static uint32_t cnt1, cnt2;
  static ButtonState st = BTNST_UNKNOWN;

  // clear one cycle states
  if ((st == BTNST_PRESSED) ||
      (st == BTNST_LONG_PRESSED)) {
    cnt1 = cnt2 = 0;
    st = BTNST_UP;
  }

  if (t != now) { // 1 ms
    t = now;
    bool press = !gpio_get(BUTTON_PIN);

    switch (st) {

      case BTNST_UNKNOWN:
        cnt1 = cnt2 = 0;
        if (!press) st = BTNST_UP;
        break;

      case BTNST_UP:
        if (press) {
          if (cnt1++ >= BTN_MIN_PULSE) {
            st = BTNST_DOWN;
          }
        }
        else cnt1 = 0;
        break;

      case BTNST_DOWN:
        if (press) {
          cnt2 = 0;
          if (cnt1++ >= BTN_HOLD_PULSE) {
            st = BTNST_HOLD;
          }
        }
        else {
          if (cnt2++ >= BTN_MIN_PULSE) {
            st = BTNST_PRESSED;
          }
        }
        break;

    
      case BTNST_HOLD:
        if (press) cnt2 = 0;
        else {
          if (cnt2++ >= BTN_MIN_PULSE) {
            st = BTNST_LONG_PRESSED;
          }
        }
        break;

      default:
        cnt1 = cnt2 = 0;
        st = BTNST_UNKNOWN;
        break;
    } // switch (st)
  } // if ms

  return st;
}
