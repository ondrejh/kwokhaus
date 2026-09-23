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

  EVENT_STATUS,

  EVENT_MOTOR_IS_UP,
  EVENT_MOTOR_IS_DOWN,
} event_t;

typedef enum {
  EVENT_RETRY,
  EVENT_HANDLED,
} event_result_t;

#define EVENT_QUEUE_SIZE 8

bool event_queue_push(event_t event);
bool event_queue_contains(event_t event);
event_t event_queue_peek(void);
event_t event_queue_pop(void);
void event_print(event_t event);
void queue_button_events(ButtonState btn_o, ButtonState btn_c, ButtonState btn_l);
//event_t get_time_event(uint32_t now, tim_t *tloc);

#endif // __EVENTS_H__
