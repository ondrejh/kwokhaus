#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "includes.h"

typedef enum {
  EVENT_NONE,

  /*EVENT_PRESS,
  EVENT_LONGPRESS,
  EVENT_TIME,
  EVENT_LOCK,
  EVENT_UNLOCK,
  EVENT_CMD_UNLOCK,*/

  EVENT_BTN_OPEN,
  EVENT_BTN_CLOSE,

  EVENT_CMD_OPEN,
  EVENT_CMD_CLOSE,
} event_t;

event_t get_button_event(ButtonState btn_o, ButtonState btn_c, uint32_t now);
//event_t get_time_event(uint32_t now, tim_t *tloc);

#endif // __EVENTS_H__
