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

#define UART_RX_BUFFLEN 64
typedef struct {
  uint8_t bufinp;
  uint8_t bufoutp;
  uint8_t buff[UART_RX_BUFFLEN];
} buff_t;

buff_t uart_rx_buff = {.bufinp = 0, .bufoutp = 0,};

// RX interrupt handler
void on_uart_rx() {
  while (uart_is_readable(UART_ID)) {
    uint8_t ch = uart_getc(UART_ID);
    uint8_t p = (uart_rx_buff.bufinp + 1) % UART_RX_BUFFLEN;
    if (p != uart_rx_buff.bufoutp) {
      uart_rx_buff.buff[p] = ch;
      uart_rx_buff.bufinp = p;
    }
    // echo (test)
    if (uart_is_writable(UART_ID)) {
      uart_putc(UART_ID, ch);
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
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);
  uart_set_irq_enables(UART_ID, true, false);
}
