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
} event_t;

event_t get_input_event(uint32_t now);

#endif // __EVENTS_H__
