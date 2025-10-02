#ifndef __RTC_H__
#define __RTC_H__

void ds3231_set_time(uint8_t hour, uint8_t min, uint8_t sec);
void ds3231_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);

#endif
