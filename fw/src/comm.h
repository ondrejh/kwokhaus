#ifndef __COMM_H__
#define __COMM_H__

void comm_init(void);
void comm_write(uint8_t *msg, int len);
bool comm_tx_busy(void);
int comm_poll(uint32_t now, uint32_t tout, uint8_t *rxbuf, int max);

#endif // __COMM_H__
