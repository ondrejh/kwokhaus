#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "includes.h"

typedef enum {
  EVENT_NONE,

  EVENT_BTN_OPEN,
  EVENT_BTN_CLOSE,

  EVENT_BTN_LIGHT,

  EVENT_CMD_OPEN,
  EVENT_CMD_CLOSE,

  EVENT_CMD_LIGHT_ON,
  EVENT_CMD_LIGHT_OFF,
} event_t;

event_t get_button_event(ButtonState btn_o, ButtonState btn_c, ButtonState btn_l, uint32_t now);
//event_t get_time_event(uint32_t now, tim_t *tloc);

#endif // __EVENTS_H__
