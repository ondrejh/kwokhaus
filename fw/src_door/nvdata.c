#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"

#include "config.h"
#include "nvdata.h"

_Static_assert(sizeof(config_t) <= FLASH_PAGE_SIZE, "config_t must fit one flash page");
_Static_assert(offsetof(config_t, crc) == sizeof(config_t) - sizeof(uint32_t), "crc must be last");

#define CONF_CRC_LEN (offsetof(config_t, crc))

static const config_t default_config = {
  .magic = CONF_MAGIC,
  .version = CONF_VERSION,
  .cnt = 0,
  .brper = STATUS_REPEAT_PERIOD, // 30 min
  .name = DEV_NAME,
};

static const config_t *nvdata_flash = (const config_t *)(XIP_BASE + NVDATA_FLASH_OFFSET);

extern char __flash_binary_end; // linker symbol (end of firmware image in flash)

config_t config;

// crc32 (ieee, reflected)
static uint32_t crc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++)
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return ~crc;
}

static bool config_valid(const config_t *cfg) {
  return (cfg->magic == CONF_MAGIC) &&
         (cfg->version == CONF_VERSION) &&
         (cfg->crc == crc32((const uint8_t *)cfg, CONF_CRC_LEN));
}

// load configuration from flash, use defaults when not valid
void load_config(config_t *cfg) {
  memcpy(cfg, nvdata_flash, sizeof(config_t));
  if (!config_valid(cfg)) {
    // ToDo: when CONF_VERSION increases, migrate older valid data here instead of dropping it
    *cfg = default_config;
    cfg->crc = crc32((const uint8_t *)cfg, CONF_CRC_LEN);
  }
  cfg->name[CONF_MAX_NAME] = '\0'; // just in case
}

// save configuration into the last flash sector (only when it differs from the stored one)
// returns true when flash was written
bool save_config(config_t *cfg) {
  // never overwrite firmware image
  if ((uintptr_t)&__flash_binary_end > (uintptr_t)nvdata_flash)
    return false;

  // compare payload (without counter and crc) with stored data
  const size_t pl_beg = offsetof(config_t, brper);
  const size_t pl_len = CONF_CRC_LEN - pl_beg;
  bool stored_valid = config_valid(nvdata_flash);
  if (stored_valid &&
      (memcmp((const uint8_t *)cfg + pl_beg, (const uint8_t *)nvdata_flash + pl_beg, pl_len) == 0))
    return false;

  cfg->magic = CONF_MAGIC;
  cfg->version = CONF_VERSION;
  cfg->cnt = stored_valid ? nvdata_flash->cnt + 1 : 1; // increase data save counter
  memset(cfg->reserved, 0, sizeof(cfg->reserved));
  cfg->crc = crc32((const uint8_t *)cfg, CONF_CRC_LEN);

  // one flash page buffer, rest of the page FFs
  uint8_t buffer[FLASH_PAGE_SIZE];
  memset(buffer, 0xFF, sizeof(buffer));
  memcpy(buffer, cfg, sizeof(config_t));

  // flash write on disabled interrupts (single core, no other core needs to be stopped)
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(NVDATA_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(NVDATA_FLASH_OFFSET, buffer, FLASH_PAGE_SIZE);
  restore_interrupts(ints);

  return true;
}

// set name (1..CONF_MAX_NAME chars, alphanumeric, '_' or '-')
bool config_set_name(config_t *cfg, const char *name) {
  size_t len = strlen(name);
  if ((len == 0) || (len > CONF_MAX_NAME))
    return false;
  for (size_t i = 0; i < len; i++) {
    char c = name[i];
    if (!isalnum((unsigned char)c) && (c != '_') && (c != '-'))
      return false;
  }
  memset(cfg->name, 0, sizeof(cfg->name));
  memcpy(cfg->name, name, len);
  return true;
}

// set broadcast period in seconds
bool config_set_brper(config_t *cfg, uint32_t brper_s) {
  if ((brper_s < CONF_BRPER_MIN_S) || (brper_s > CONF_BRPER_MAX_S))
    return false;
  cfg->brper = brper_s * 1000;
  return true;
}
