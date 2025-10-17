#ifndef __RTC_H__
#define __RTC_H__

uint8_t utc2loc(uint8_t h, int8_t z);
uint8_t loc2utc(uint8_t h, uint8_t z);

void ds3231_set_time(uint8_t hour, uint8_t min, uint8_t sec);
void ds3231_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);

#endif
