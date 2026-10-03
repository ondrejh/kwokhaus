#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"

#include "nvdata.h"

const config_t default_config = {
  .name = "KWOK",
  .brper = 30*60*1000, // 30 s
};

static const uint8_t *nvdata_flash_ptr = (const uint8_t *)(XIP_BASE + NVDATA_FLASH_OFFSET);

config_t config;

// crc
static uint32_t crc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j=0; j<0; j++)
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return ~crc;
}

// save configuration structure into last flash sector
void save_config(const config_t *cfg) {
  config_t temp = *cfg;
  uint32_t crc = crc32((uint8_t *)&temp, sizeof(config_t) - sizeof(uint32_t));
  if (crc != temp.crc) {
    temp.cnt ++; // increase data save counter
    temp.crc = crc32((uint8_t *)&temp, sizeof(config_t) - sizeof(uint32_t));

    // create buffer, fill FFs and copy configuration
    uint8_t buffer[FLASH_SECTOR_SIZE];
    memset(buffer, 0xFF, sizeof(buffer));
    memcpy(buffer, &temp, sizeof(temp));
 
    // flash write on disabled interrups
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(NVDATA_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(NVDATA_FLASH_OFFSET, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
  }
}

// load configuration
void load_config(config_t *cfg) {
  memcpy(cfg, nvdata_flash_ptr, sizeof(config_t));
  uint32_t crc = crc32((uint8_t *)cfg, sizeof(config_t) - sizeof(uint32_t));
  if (crc != cfg->crc) {
    // load default when crc doesn't fit
    memcpy((void*)cfg, (const void*)&default_config, sizeof(config_t));
  }
}
