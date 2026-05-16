#include "includes.h"

LockState lock = LOCK_UNKNOWN;

LockState lock_poll(uint32_t now) {
  //static LockState lock = LOCK_UNKNOWN;
  static int cnt = 0;
  static uint32_t t = 0;

  if (t != now) {
    t = now;
    bool lck = gpio_get(LOCK_PIN);
    if (!lck && (lock != LOCK_LOCKED)) {
      cnt ++;
      if (cnt >= LOCK_FILTER_T)
        lock = LOCK_LOCKED;
    }
    else if (lck && (lock != LOCK_UNLOCKED)) {
      cnt ++;
      if (cnt >= LOCK_FILTER_T)
        lock = LOCK_UNLOCKED;
    }
    else
      cnt = 0;
  }
  
  return lock;
}

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

PirState pir_poll(uint32_t now) {
  static PirState st = PIRST_UNKNOWN;
  static uint32_t tout;
  bool pir = gpio_get(PIR_INPUT_PIN);

  // clear special states
  if (st == PIRST_FREE_NOW)
    st = PIRST_FREE;
  if (st == PIRST_OCUPY_NOW)
    st = PIRST_OCUPY;

  // get state  
  switch (st) {
    case PIRST_UNKNOWN:
      st = pir ? PIRST_OCUPY : PIRST_FREE;
      tout = now;
      break;
    case PIRST_OCUPY:
      //gpio_put(LED_GREEN_PIN, true);
      if (pir)
        tout = now;
      else if ((now - tout) > PIR_RELEASE_TIMEOUT)
        st = PIRST_FREE_NOW;
      break;
    case PIRST_FREE:
      //gpio_put(LED_GREEN_PIN, false);
      if (!pir)
        tout = now;
      else if ((now - tout) > PIR_OCUPY_TIMEOUT)
        st = PIRST_OCUPY_NOW;
      break;
    default:
      st = PIRST_UNKNOWN;
      break;
  }
  
  return st;
}