#include "includes.h"

event_t get_button_event(ButtonState btn_o, ButtonState btn_c, ButtonState btn_l, uint32_t now) {
  event_t event = EVENT_NONE;

  if (btn_o == BTNST_PRESSED)
    event = EVENT_BTN_OPEN;
  else if (btn_c == BTNST_PRESSED)
    event = EVENT_BTN_CLOSE;
  else if (btn_l == BTNST_PRESSED)
    event = EVENT_BTN_LIGHT;
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