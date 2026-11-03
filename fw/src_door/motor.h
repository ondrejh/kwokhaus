#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <stdint.h>
#include <stdbool.h>

// commands sent from main logic to the motor task
typedef enum {
  MOTOR_CMD_OPEN,          // start opening if motor is idle
  MOTOR_CMD_CLOSE,         // start closing if motor is idle
  MOTOR_CMD_OPEN_OR_STOP,  // stop if running, start opening otherwise (button)
  MOTOR_CMD_CLOSE_OR_STOP, // stop if running, start closing otherwise (button)
  MOTOR_CMD_FORCE_UP,      // run up while forced (button hold)
  MOTOR_CMD_FORCE_DOWN,    // run down while forced (button hold)
  MOTOR_CMD_FORCE_OFF,     // release force
} motor_cmd_t;

#define MOTOR_CMD_QUEUE_LEN 8

void motor_init(void);
void motor_update(uint32_t now);

bool motor_command(motor_cmd_t cmd);
uint8_t motor_get_status(void);
uint32_t motor_get_pwm(void);
GateState motor_get_gate(void);

#endif // __MOTOR_H__
