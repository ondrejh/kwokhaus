#ifndef __COMM_H__
#define __COMM_H__

int strip(uint8_t *buff, int len);

void comm_init(void);
void comm_write(uint8_t *msg, int len);
bool comm_tx_busy(void);
int comm_poll(uint32_t now, uint32_t tout, uint8_t *rxbuf, int max);
int comm_parse(uint8_t *buff, int len, int max, event_t *event);
int sprint_status(uint8_t *buff, int max);

#endif // __COMM_H__
