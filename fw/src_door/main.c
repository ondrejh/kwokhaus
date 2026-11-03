#include "includes.h"

#define MAIN_TASK_STACK 1024
#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define IO_TASK_STACK 1024
#define IO_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#define COMM_BUFLEN 128
static uint8_t comm_buff[COMM_BUFLEN];
static uint8_t usb_comm_buff[COMM_BUFLEN];

// status reporting state
static uint8_t motor_status_last = 0;
static bool light_last = false;
static uint32_t tLastTx = 0;
static bool status_sent = false;
static uint32_t status_change_t = 0;
static bool status_change_pending = true;
static uint32_t status_change_timeout = 0;

// light logic state (output itself is driven by the io task)
static bool light = false;
static uint32_t lightDime = LIGHT_DIMMING;
static uint32_t tLightOff = 0;
static uint32_t tLightOn = 0;
static bool lightHold = false; // don't start auto off timer after motor stops

// configuration changed (save to flash when motor is idle)
static bool config_dirty = false;

void init(void) {
  // Initialize nvdata (load configuration)
  load_config(&config);

  // Initialize communication
  comm_init();

  // Initialize motor, light and status LED outputs
  io_init();
}

// receive commands from UART and USB, parse them into events
static void comm_receive(uint32_t now) {
  int comrx = comm_poll(now, 100, comm_buff, COMM_BUFLEN);
  if (comrx) {
    comm_buff[comrx] = '\0';
    printf("Rx: %.*s\n", COMM_BUFLEN, comm_buff);
    comm_parse(comm_buff, comrx, COMM_BUFLEN);
  }

  int usb_comrx = usb_comm_poll(now, 100, usb_comm_buff, COMM_BUFLEN);
  if (usb_comrx) {
    usb_comm_buff[usb_comrx] = '\0';
    printf("USB Rx: %.*s\n", COMM_BUFLEN, usb_comm_buff);
    // configuration commands are accepted from USB only
    if (!usb_cmd_parse(usb_comm_buff, usb_comrx, COMM_BUFLEN, &config_dirty))
      comm_parse(usb_comm_buff, usb_comrx, COMM_BUFLEN);
  }
}

// save changed configuration (flash erase blocks interrupts ~50 ms, do it with motor stopped)
static void config_update(void) {
  if (config_dirty && (motor_get_pwm() == 0)) {
    config_dirty = false;
    if (save_config(&config))
      printf("CFG: saved (cnt %lu)\n", (unsigned long)config.cnt);
  }
}

// detect status change and queue status events (on change and periodically)
static void status_update(uint32_t now) {
  uint8_t motor_status = motor_get_status();

  if ((motor_status != motor_status_last) || (light != light_last)) {
    motor_status_last = motor_status;
    light_last = light;
    status_change_t = now;
    status_change_pending = true;
    status_change_timeout = STATUS_CHANGE_TIMEOUT;
  }
  else if (status_change_pending &&
           ((now - status_change_t) >= status_change_timeout)) {
    if (event_queue_push(EVENT_STATUS) || event_queue_contains(EVENT_STATUS))
      status_change_pending = false;
  }

  if ((config.brper != 0) && ((now - tLastTx) > config.brper))
    event_queue_push(EVENT_STATUS);
}

// send status message, retry later if uart is busy or the last one was too recent
static event_result_t status_send(uint32_t now) {
  if (comm_tx_busy() ||
      (status_sent && ((now - tLastTx) < STATUS_REPEAT_PERIOD_MIN)))
    return EVENT_RETRY;

  tLastTx = now;
  status_sent = true;
  status_change_timeout = STATUS_CHANGE_TIMEOUT;
  int len = sprint_status(comm_buff, COMM_BUFLEN);
  comm_write(comm_buff, len);
  printf("TX: %s\n", comm_buff);
  return EVENT_HANDLED;
}

// light on/off decisions: commands, button, auto off timers
static void light_handle_event(event_t event, uint32_t now) {
  switch (event) {
    case EVENT_CMD_LIGHT_ON:
      light = true;
      lightDime = LIGHT_DIMMING;
      tLightOff = 0; // switch off automatic switching off, clear?
      break;
    case EVENT_CMD_LIGHT_OFF:
      light = false;
      lightDime = LIGHT_DIMMING;
      lightHold = false;
      break;
    case EVENT_CMD_LIGHT_HOLD:
      lightHold = true;
      tLightOff = 0;
      break;
    case EVENT_BTN_LIGHT:
      light = !light;
      lightHold = false;
      lightDime = LIGHT_DIMMING_BTN;
      if (light) {
        tLightOn = now;
        tLightOff = LIGHT_AUTO_OFF;
      }
      break;
    case EVENT_BTN_OPEN:
    case EVENT_BTN_CLOSE:
    case EVENT_CMD_OPEN:
    case EVENT_CMD_CLOSE:
      // any new motor command cancels light hold (re-set by EVENT_CMD_LIGHT_HOLD)
      lightHold = false;
      break;
    case EVENT_MOTOR_IS_UP:
    case EVENT_MOTOR_IS_DOWN:
      // motor reached end position - start light timer if light is on
      if (light && !lightHold) {
        tLightOn = now;
        tLightOff = LIGHT_DOOR_AUTO_OFF;
        lightDime = LIGHT_DIMMING; // slow dimming
      }
      break;
    default:
      break;
  }

  // automatic light off
  if (light && (tLightOff != 0) && ((now - tLightOn) > tLightOff))
    light = false;

  light_set(light, lightDime);
}

// translate events to motor commands
static void motor_handle_event(event_t event) {
  switch (event) {
    case EVENT_BTN_OPEN:
      motor_command(MOTOR_CMD_OPEN_OR_STOP);
      break;
    case EVENT_BTN_CLOSE:
      motor_command(MOTOR_CMD_CLOSE_OR_STOP);
      break;
    case EVENT_CMD_OPEN:
      motor_command(MOTOR_CMD_OPEN);
      break;
    case EVENT_CMD_CLOSE:
      motor_command(MOTOR_CMD_CLOSE);
      break;
    default:
      break;
  }
}

// force motor up/down while button is held
static void motor_handle_force(const btn_ctx_t *btnL, const btn_ctx_t *btnR) {
  static motor_cmd_t force_last = MOTOR_CMD_FORCE_OFF;

  motor_cmd_t force = MOTOR_CMD_FORCE_OFF;
  if (btnL->st == BTNST_HOLD)
    force = MOTOR_CMD_FORCE_UP;
  else if (btnR->st == BTNST_HOLD)
    force = MOTOR_CMD_FORCE_DOWN;

  if ((force != force_last) && motor_command(force))
    force_last = force;
}

// main logic task: communication, buttons, events, status reporting, light logic
void main_task(void *param) {
  (void)param;

  btn_ctx_t btnL, btnR, btnLight;
  button_init(&btnL, BUTTON_L_PIN);
  button_init(&btnR, BUTTON_R_PIN);
  button_init(&btnLight, BUTTON_LIGHT_PIN);

  status_change_t = millis();
  status_change_timeout = 1000 + (get_rand_32() % STATUS_CHANGE_TIMEOUT);

  while (true) {
    vTaskDelay(1);
    uint32_t now = millis();

    comm_receive(now);
    config_update();
    status_update(now);

    // button polling and event generation
    button_poll(&btnL, now);
    button_poll(&btnR, now);
    button_poll(&btnLight, now);
    queue_button_events(btnL.st, btnR.st, btnLight.st);
    motor_handle_force(&btnL, &btnR);

    // event processing
    event_t event = event_queue_peek();
    event_result_t event_result = EVENT_HANDLED;

    if (event == EVENT_STATUS)
      event_result = status_send(now);

    light_handle_event(event, now);
    motor_handle_event(event);

    if (event_result == EVENT_HANDLED) {
      event_queue_pop();
      event_print(event);
    }
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name) {
  (void)task;
  panic("stack overflow: %s", name);
}

void vApplicationMallocFailedHook(void) {
  panic("malloc failed");
}

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  xTaskCreate(io_task, "io", IO_TASK_STACK, NULL, IO_TASK_PRIORITY, NULL);
  xTaskCreate(main_task, "main", MAIN_TASK_STACK, NULL, MAIN_TASK_PRIORITY, NULL);

  vTaskStartScheduler();

  // should never get here
  while (true)
    tight_loop_contents();
}
