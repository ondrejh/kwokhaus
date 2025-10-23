#ifndef __NVDATA_H__
#define __NVDATA_H__

#define CONF_MAX_NAME 8

typedef struct {
  uint8_t name[CONF_MAX_NAME + 1];
  int8_t zone;
} config_t;

extern config_t config;

void config_init(void);

#endif // __NVDATA_H__
