#include "includes.h"

const config_t default_config = {
  .name = "KWAK",
  .zone = 2,
};

config_t config;

void config_init(void) {
  memcpy((void*)&config, (const void*)&default_config, sizeof(config));
}
