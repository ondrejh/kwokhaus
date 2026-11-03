#ifndef __NVDATA_H__
#define __NVDATA_H__

#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// configuration lives in the last flash sector, firmware (uf2) update doesn't touch it
// (FLASH_SECTOR_SIZE and FLASH_PAGE_SIZE are defined in the sdk)
#define NVDATA_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

#define CONF_MAGIC 0x434F4E46 // "CONF"
#define CONF_VERSION 1 // increase when config_t layout changes (and handle migration in load_config)

#define CONF_MAX_NAME 16

// device configuration structure
typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t cnt; // data storage counter
  uint32_t brper; // broadcast (status repeat) period [ms]
  char name[CONF_MAX_NAME + 1]; // call name (zero terminated)
  uint8_t reserved[3]; // explicit padding (keeps crc deterministic)
  uint32_t crc; // checksum (must be last)
} config_t;

extern config_t config;

void load_config(config_t *cfg);
bool save_config(config_t *cfg);

bool config_set_name(config_t *cfg, const char *name);
bool config_set_brper(config_t *cfg, uint32_t brper_s);

#endif // __NVDATA_H__
