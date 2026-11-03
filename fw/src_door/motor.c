#include "includes.h"

// motor control: ADC sampling, motor driver outputs, current monitoring
// (end position detection) and safety timeout
// motor_update() is called periodically from the io task

static QueueHandle_t motor_cmd_queue = NULL;

// state shared with the main task (written by the motor task only)
static volatile uint8_t motor_status = 0;
static volatile uint32_t motor_pwm = 0;
static volatile GateState gate = GATE_UNKNOWN;

void motor_init(void) {
  // driver enable outputs
  gpio_init(ENABLE_R_PIN);
  gpio_set_dir(ENABLE_R_PIN, GPIO_OUT);
  gpio_init(ENABLE_L_PIN);
  gpio_set_dir(ENABLE_L_PIN, GPIO_OUT);

  // driver PWM outputs
  gpio_set_function(PWM_L_PIN, GPIO_FUNC_PWM);
  gpio_set_function(PWM_R_PIN, GPIO_FUNC_PWM);
  uint sliceL = pwm_gpio_to_slice_num(PWM_L_PIN);
  uint sliceR = pwm_gpio_to_slice_num(PWM_R_PIN);
  pwm_set_wrap(sliceL, PWM_PERIOD);
  pwm_set_wrap(sliceR, PWM_PERIOD);
  pwm_set_gpio_level(PWM_L_PIN, 0);
  pwm_set_gpio_level(PWM_R_PIN, 0);
  pwm_set_enabled(sliceL, true);
  pwm_set_enabled(sliceR, true);

  adc_module_init();

  motor_cmd_queue = xQueueCreate(MOTOR_CMD_QUEUE_LEN, sizeof(motor_cmd_t));
  configASSERT(motor_cmd_queue != NULL);
}

bool motor_command(motor_cmd_t cmd) {
  return xQueueSend(motor_cmd_queue, &cmd, 0) == pdTRUE;
}

uint8_t motor_get_status(void) {
  return motor_status;
}

uint32_t motor_get_pwm(void) {
  return motor_pwm;
}

GateState motor_get_gate(void) {
  return gate;
}

static void motor_outputs(uint8_t status, uint32_t pwm) {
  if (status & (MOTOR_GO_UP | MOTOR_FORCE_UP)) {
    gpio_put(ENABLE_L_PIN, true);
    gpio_put(ENABLE_R_PIN, true);
    pwm_set_gpio_level(PWM_R_PIN, pwm);
    pwm_set_gpio_level(PWM_L_PIN, 0);
  }
  else if (status & (MOTOR_GO_DOWN | MOTOR_FORCE_DOWN)) {
    gpio_put(ENABLE_L_PIN, true);
    gpio_put(ENABLE_R_PIN, true);
    pwm_set_gpio_level(PWM_R_PIN, 0);
    pwm_set_gpio_level(PWM_L_PIN, pwm);
  }
  else {
    gpio_put(ENABLE_L_PIN, false);
    gpio_put(ENABLE_R_PIN, false);
    pwm_set_gpio_level(PWM_R_PIN, 0);
    pwm_set_gpio_level(PWM_L_PIN, 0);
  }
}

// motor state (owned by the io task)
static uint8_t status = 0;
static uint8_t force = 0; // MOTOR_FORCE_UP / MOTOR_FORCE_DOWN / 0
static int32_t adc[3] = {0, 0, 0};
static uint32_t pwm = 0;
static uint16_t voltage = 0;

static uint32_t motor_running_t = 0;
static uint32_t motor_current_t = 0;
static int32_t motor_current_offset[2] = {0, 0};
static uint32_t motor_current_offset_t = 0;
static bool motor_current_offset_calibrated = false;
static bool motor_current_measured = false;
#ifdef DEBUG
static uint32_t tAdc = 0;
#endif

void motor_update(uint32_t now) {
  // process commands from main logic
  motor_cmd_t cmd;
  while (xQueueReceive(motor_cmd_queue, &cmd, 0) == pdTRUE) {
    switch (cmd) {
      case MOTOR_CMD_OPEN_OR_STOP:
      case MOTOR_CMD_CLOSE_OR_STOP:
        if (status & MOTOR_RUNNING) {
          printf("MOTOR STOP\n");
          status &= ~MOTOR_RUNNING;
        }
        else {
          status |= (cmd == MOTOR_CMD_OPEN_OR_STOP) ? MOTOR_GO_UP : MOTOR_GO_DOWN;
          motor_running_t = now;
        }
        break;
      case MOTOR_CMD_OPEN:
      case MOTOR_CMD_CLOSE:
        if ((status & MOTOR_RUNNING) == 0) {
          status |= (cmd == MOTOR_CMD_OPEN) ? MOTOR_GO_UP : MOTOR_GO_DOWN;
          motor_running_t = now;
        }
        break;
      case MOTOR_CMD_FORCE_UP:
        force = MOTOR_FORCE_UP;
        break;
      case MOTOR_CMD_FORCE_DOWN:
        force = MOTOR_FORCE_DOWN;
        break;
      case MOTOR_CMD_FORCE_OFF:
        force = 0;
        break;
    }
  }

  // safety timeout
  if (status & MOTOR_RUNNING) {
    if ((now - motor_running_t) > MOTOR_SAFETY_TIMEOUT) {
      printf("MOTOR TIMEOUT\n");
      status &= ~MOTOR_RUNNING;
    }
  }

  // force motor up/down
  if (force == MOTOR_FORCE_UP) {
    status |= MOTOR_FORCE_UP;
    status &= ~(MOTOR_GO_DOWN | MOTOR_FORCE_DOWN | MOTOR_GO_UP);
  }
  else if (force == MOTOR_FORCE_DOWN) {
    status |= MOTOR_FORCE_DOWN;
    status &= ~(MOTOR_GO_UP | MOTOR_FORCE_UP | MOTOR_GO_DOWN);
  }
  else {
    status &= ~(MOTOR_FORCE_UP | MOTOR_FORCE_DOWN);
  }

  motor_outputs(status, motor_current_offset_calibrated ? pwm : 0);

  // read ADC and calculate voltage and PWM value
  if (adc_poll(now, adc)) {
    voltage = adc2u(adc[0]);
    pwm = v2pwm(voltage);
    motor_pwm = pwm;
  }

#ifdef DEBUG
  if ((now - tAdc) > 20) {
    tAdc = now;
    printf("%04X %04X %04X\n", (unsigned int)adc[0], (unsigned int)adc[1], (unsigned int)adc[2]);
  }
#endif

  // motor current monitoring
  if (status & MOTOR_RUNNING) {
    status &= ~MOTOR_IS_END;
    gate = GATE_UNKNOWN;
    if (!motor_current_offset_calibrated) { // calibrate motor current offset first
      motor_current_t = now;
      if ((now - motor_current_offset_t) > MOTOR_CURRENT_OFFSET_TIMEOUT) {
        motor_current_offset[0] = adc[1];
        motor_current_offset[1] = adc[2];
        motor_current_offset_calibrated = true;
        printf("MOTOR CURRENT OFFSET: %04X %04X\n", (unsigned int)motor_current_offset[0], (unsigned int)motor_current_offset[1]);
        motor_current_offset_t = now;
        motor_current_measured = false;
      }
    }
    else {
      if (!motor_current_measured && ((now - motor_current_offset_t) > MOTOR_CURRENT_OFFSET_TIMEOUT)) {
        int32_t current1 = (int32_t)adc[1] - motor_current_offset[0];
        int32_t current2 = (int32_t)adc[2] - motor_current_offset[1];
        printf("MOTOR CURRENT: %ld %ld\n", (long)current1, (long)current2);
        motor_current_measured = true;
      }
      if ((adc[1] > (motor_current_offset[0] + CURRENT1_MIN)) || (adc[2] > (motor_current_offset[1] + CURRENT2_MIN))) {
        motor_current_t = now;
      }
      else if ((now - motor_current_t) > MOTOR_CURRENT_TIMEOUT) {
        // no current -> end position reached
        if (status & MOTOR_GO_UP) {
          status |= MOTOR_IS_UP;
          gate = GATE_OPEN;
          event_queue_push(EVENT_MOTOR_IS_UP);
        }
        else if (status & MOTOR_GO_DOWN) {
          status |= MOTOR_IS_DOWN;
          gate = GATE_CLOSED;
          event_queue_push(EVENT_MOTOR_IS_DOWN);
        }
        status &= ~MOTOR_RUNNING;
      }
    }
  }
  else {
    motor_current_t = now;
    motor_current_offset_t = now;
    motor_current_offset_calibrated = false;
  }

  motor_status = status;
}
