#ifndef __IO_H__
#define __IO_H__

#define IO_TASK_PERIOD_MS 1
#define LED_UPDATE_PERIOD 250 // ms (status LED blink period)

void io_init(void);
void io_task(void *param);

#endif // __IO_H__
