#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "includes.h"

typedef enum {
  EVENT_NONE,
  EVENT_PRESS,
  EVENT_LONGPRESS,
  EVENT_TIME,
  EVENT_LOCK,
  EVENT_UNLOCK,
  EVENT_CMD_UNLOCK,
  EVENT_OCUPY,
  EVENT_FREE,
  EVENT_CMD_LIGHT,
} event_t;

event_t get_input_event(uint32_t now);
//event_t get_time_event(uint32_t now, tim_t *tloc);

#endif // __EVENTS_H__
