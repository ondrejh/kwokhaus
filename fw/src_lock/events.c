#include "includes.h"

event_t get_input_event(uint32_t now) {
  event_t event = EVENT_NONE;

  // collect lock events
  if (event == EVENT_NONE) {
    static LockState lock = LOCK_UNKNOWN;
    LockState lck = lock_poll(now);
    if (lock != lck) {
      if (lck == LOCK_LOCKED)
        event = EVENT_LOCK;
      else if (lck == LOCK_UNLOCKED)
        event = EVENT_UNLOCK;
    }
    lock = lck;
  }

  // collect button events
  if (event == EVENT_NONE) {
    ButtonState btn = button_poll(now);
    if (btn == BTNST_PRESSED)
      event = EVENT_PRESS;
    else if (btn == BTNST_LONG_PRESSED)
      event = EVENT_LONGPRESS;
  }

  // collect pir events
  if (event == EVENT_NONE) {
    PirState pir = pir_poll(now);
    if (pir == PIRST_FREE_NOW) {
      //gpio_put(LED_GREEN_PIN, false);
      event = EVENT_FREE;
    }
    else if (pir == PIRST_OCUPY_NOW) {
      //gpio_put(LED_GREEN_PIN, true);
      event = EVENT_OCUPY;
    }
  }

  return event;
}

/*event_t get_time_event(uint32_t now, tim_t *tloc) {
  static uint32_t tTim = 0;
  event_t event = EVENT_NONE;

  if ((now -tTim) >= TIME_POLLING_PERIOD) {
    int8_t h, m;

    // get utc time from rtc module
    ds3231_get_time(&h, &m, &tloc->s);

    // calculate local time
    h = utc2loc(h, tloc->z);

    // check for minute event
    if ((m != tloc->m) || (h != tloc->h))
      event = EVENT_TIME;

    // save it for the next time
    tloc->h = h;
    tloc->m = m;
    tTim = now;
  }

  return event;
}*/