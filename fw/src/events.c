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

  return event;
}
