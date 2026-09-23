#include "includes.h"

static event_t event_queue[EVENT_QUEUE_SIZE];
static uint8_t event_queue_head = 0;
static uint8_t event_queue_tail = 0;
static uint8_t event_queue_count = 0;

bool event_queue_push(event_t event) {
  if ((event == EVENT_NONE) || (event_queue_count >= EVENT_QUEUE_SIZE))
    return false;

  event_queue[event_queue_tail] = event;
  event_queue_tail = (event_queue_tail + 1) % EVENT_QUEUE_SIZE;
  event_queue_count++;
  return true;
}

event_t event_queue_peek(void) {
  if (event_queue_count == 0)
    return EVENT_NONE;

  return event_queue[event_queue_head];
}

event_t event_queue_pop(void) {
  event_t event;

  if (event_queue_count == 0)
    return EVENT_NONE;

  event = event_queue[event_queue_head];
  event_queue_head = (event_queue_head + 1) % EVENT_QUEUE_SIZE;
  event_queue_count--;
  return event;
}

void event_print(event_t event) {
  switch (event) {
    case EVENT_BTN_OPEN:
      printf("EVENT: BUTTON OPEN\n");
      break;
    case EVENT_BTN_CLOSE:
      printf("EVENT: BUTTON CLOSE\n");
      break;
    case EVENT_BTN_LIGHT:
      printf("EVENT: BUTTON LIGHT\n");
      break;
    case EVENT_CMD_OPEN:
      printf("EVENT: COMMAND OPEN\n");
      break;
    case EVENT_CMD_CLOSE:
      printf("EVENT: COMMAND CLOSE\n");
      break;
    case EVENT_CMD_LIGHT_ON:
      printf("EVENT: COMMAND LIGHT ON\n");
      break;
    case EVENT_CMD_LIGHT_OFF:
      printf("EVENT: COMMAND LIGHT OFF\n");
      break;
    case EVENT_STATUS:
      printf("EVENT: STATUS\n");
      break;
    case EVENT_MOTOR_IS_UP:
      printf("EVENT: MOTOR UP\n");
      break;
    case EVENT_MOTOR_IS_DOWN:
      printf("EVENT: MOTOR DOWN\n");
      break;
    case EVENT_NONE:
      break;
  }
}

void queue_button_events(ButtonState btn_o, ButtonState btn_c, ButtonState btn_l) {
  if (btn_o == BTNST_PRESSED)
    event_queue_push(EVENT_BTN_OPEN);
  if (btn_c == BTNST_PRESSED)
    event_queue_push(EVENT_BTN_CLOSE);
  if (btn_l == BTNST_PRESSED)
    event_queue_push(EVENT_BTN_LIGHT);
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