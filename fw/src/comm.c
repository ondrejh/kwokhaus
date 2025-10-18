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
    if (ch == '\n')
      break;
  }
  if ((ch == '\n') || ((bufp > 0) && ((now - trx) > tout))) {
    int ret = bufp;
    bufp = 0;
    return ret;
  }
  return 0;
}
