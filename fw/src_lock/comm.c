#include "includes.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_BUFFLEN 256
typedef struct {
  volatile int bufinp;
  volatile int bufoutp;
  uint8_t buff[UART_BUFFLEN];
} buff_t;

// collect all the states needed to export
//extern tim_t tloc;
extern LockState lock;

buff_t uart_rx_buff = {.bufinp = 0, .bufoutp = 0,};
buff_t uart_tx_buff = {.bufinp = 0, .bufoutp = 0,};
volatile bool tx_busy = false;

// uart interrupt handler
void on_uart_irq() {
  while (uart_is_readable(UART_ID)) {
    uint8_t ch = uart_getc(UART_ID);
    int p = (uart_rx_buff.bufinp + 1) % UART_BUFFLEN;
    if (p != uart_rx_buff.bufoutp) {
      uart_rx_buff.buff[p] = ch;
      uart_rx_buff.bufinp = p;
    }
  }

  if (uart_is_writable(UART_ID)) {
    if (uart_tx_buff.bufinp != uart_tx_buff.bufoutp) {
      int p = (uart_tx_buff.bufoutp + 1) % UART_BUFFLEN;
      uart_putc_raw(UART_ID, uart_tx_buff.buff[p]);
      uart_tx_buff.bufoutp = p;
      if (!tx_busy) {
        tx_busy = true;
        uart_set_irq_enables(UART_ID, true, true);
      }
    }
    else {
      tx_busy = false;
      uart_set_irq_enables(UART_ID, true, false);
    }
  }
}

void comm_init(void) {
  uart_init(UART_ID, BAUD_RATE);
  
  gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
  gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

  uart_set_hw_flow(UART_ID, false, false);
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
  uart_set_fifo_enabled(UART_ID, false);

  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_exclusive_handler(UART_IRQ, on_uart_irq);
  irq_set_enabled(UART_IRQ, true);
  uart_set_irq_enables(UART_ID, true, false);
}

void comm_write(uint8_t *msg, int len) {
  for (int i = 0; i < len; i++) {
    uint8_t ch = *msg++;
    int p = (uart_tx_buff.bufinp + 1) % UART_BUFFLEN;
    if (p != uart_tx_buff.bufoutp) {
      uart_tx_buff.buff[p] = ch;
      uart_tx_buff.bufinp = p;
    }
    if (!tx_busy) {
      on_uart_irq();
    }
  }
}

bool comm_tx_busy(void) {
  return tx_busy;
}

// tight loop function
// check if there is a new char received
// in case of timeout return buffer of received characters
int comm_poll(uint32_t now, uint32_t tout, uint8_t *rxbuf, int max) {
  static int bufp = 0;
  static uint32_t trx = 0;
  uint8_t ch = 0;
  while (uart_rx_buff.bufinp != uart_rx_buff.bufoutp) {
    int p = (uart_rx_buff.bufoutp + 1) % UART_BUFFLEN;
    ch = uart_rx_buff.buff[p];
    uart_rx_buff.bufoutp = p;
    if (bufp >= max)
      bufp = max - 1;
    rxbuf[bufp++] = ch;
    trx = now;
  }
  if ((bufp > 0) && ((now - trx) > tout)) {
    int ret = bufp;
    bufp = 0;
    return ret;
  }
  return 0;
}

// print status into the buffer
int sprint_status(uint8_t *buff, int max) {
  int len = snprintf(buff, max, "%s: ", DEV_NAME);
  len += snprintf(&buff[len], max - len, "%s", lock==LOCK_UNLOCKED? "UNLOCKED" : "LOCKED");
  return len;
}

// parse input data, use it, maybe create output (same buffer)
int comm_parse(uint8_t *buff, int len, int max, event_t *event) {
  if ((len <= 0) || (len >= max))
    return 0;

  buff[max-1] = '\0'; // just in case

  // search for name including ": <name>" 
  uint8_t name_plus[16];
  snprintf(name_plus, 16, ": %s", DEV_NAME);
  uint8_t *p = strstr(buff, name_plus);
  if (p == NULL) // if not found bye
    return 0;
  // skip name
  p += strlen(name_plus);

  int nlen = 0;
  while(p < buff + max) {
    uint8_t ch = *p++;
    switch (ch) {
      case '?': // get status
        nlen = sprint_status(buff, max);
        break;
      case 'U':
        //printf("UNLOCK\n");
        if (event != NULL)
          *event = EVENT_CMD_UNLOCK;
        break;
      case 'L':
        if (event != NULL)
          *event = EVENT_CMD_LIGHT;
        break;
      default:
        break;
    }

    if (nlen != 0)
      break;
  }
  return nlen;
}

// is it printable character?
bool is_printable_ascii(uint8_t c) {
  if (c < 0x20)
    return false;
  if (c > 0x7E)
    return false;
  return true;
}

// strip buffer content from non printable characters
int strip(uint8_t *buff, int len) {
  int beg, end, nlen = 0;
  for (beg = 0; beg < len; beg ++) {
    if (is_printable_ascii(buff[beg]))
      break;
  }
  for (end = len-1; end >= 0; end --) {
    if (is_printable_ascii(buff[end]))
      break;
  }
  if (beg < end) {
    nlen = end + 1 - beg;
    memmove(buff, &buff[beg], nlen);
  }
  
  buff[nlen] = '\0';
  return nlen;
}

