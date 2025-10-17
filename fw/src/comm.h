#ifndef __COMM_H__
#define __COMM_H__

void comm_init(void);
void comm_write(uint8_t *msg, int len);
bool comm_tx_busy(void);

#endif // __COMM_H__
