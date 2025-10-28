#ifndef __NVDATA_H__
#define __NVDATA_H__

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// size of one flash sector (do not change - pico specific)
//#define FLASH_SECTOR_SIZE (4096) .. already defined in sdk
#define NVDATA_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

#define CONF_MAX_NAME 16

// device configuration structure
typedef struct {
  uint32_t version;
  uint32_t cnt; // data storage counter
  uint32_t brper; // broadcast period
  tim_t topen; // time to open
  uint8_t name[CONF_MAX_NAME + 1]; // call name
  int8_t zone; // time zone (including daylight saving)
  uint32_t crc; // checksum
} config_t;

extern config_t config;

void save_config(const config_t *cfg);
void load_config(config_t *cfg);

#endif // __NVDATA_H__
