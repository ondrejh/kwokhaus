#include "includes.h"

// Send function for U8g2
uint8_t u8x8_byte_pico_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  switch(msg) {
    case U8X8_MSG_BYTE_INIT:
    case U8X8_MSG_BYTE_START_TRANSFER:
    case U8X8_MSG_BYTE_END_TRANSFER:
      return 1;
    case U8X8_MSG_BYTE_SEND:
      i2c_write_blocking(DISP_I2C_PORT, u8x8_GetI2CAddress(u8x8) >> 1, arg_ptr, arg_int, false);
      return 1;
    default:
      return 0;
  }
}

// Delay handler for U8g2
uint8_t u8x8_gpio_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  switch(msg) {
    case U8X8_MSG_DELAY_MILLI:
      sleep_ms(arg_int);
      return 1;
    default:
      return 0;
  }
}

// I2C Port scanning
void i2c_bus_scan(i2c_inst_t *port) {
  printf("\nI2C Bus Scan\n");
  printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
  for (int addr = 0; addr < (1 << 7); ++addr) {
    if (addr % 16 == 0)
      printf("%02x ", addr);
    
    int ret;
    uint8_t rxdata;
    //if (((addr & 0x78) == 0) || ((addr & 0x78) == 0x78))
    //  ret = PICO_ERROR_GENERIC;
    //else
      ret = i2c_read_blocking(port, addr, &rxdata, 1, false);

    printf(ret < 0 ? "." : "@");
    printf(addr % 16 == 15 ? "\n" : "  ");
  }
  printf("Done.\n");
}
