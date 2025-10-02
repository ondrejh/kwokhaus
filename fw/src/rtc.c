#include "includes.h"

#define DS3231_ADDR 0x68

// Pomocné funkce: BCD <-> binární
uint8_t bcd2dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

// Zápis času do DS3231
void ds3231_set_time(uint8_t hour, uint8_t min, uint8_t sec) {
    uint8_t buf[4];
    buf[0] = 0x00; // adresa registru seconds
    buf[1] = dec2bcd(sec);
    buf[2] = dec2bcd(min);
    buf[3] = dec2bcd(hour);
    i2c_write_blocking(RTC_I2C_PORT, DS3231_ADDR, buf, 4, false);
}

// Čtení času z DS3231
void ds3231_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec) {
    uint8_t reg = 0x00;
    uint8_t data[3];

    // nastavit registr na 0x00 (sekundy)
    i2c_write_blocking(RTC_I2C_PORT, DS3231_ADDR, &reg, 1, true);
    i2c_read_blocking(RTC_I2C_PORT, DS3231_ADDR, data, 3, false);

    *sec  = bcd2dec(data[0] & 0x7F);
    *min  = bcd2dec(data[1]);
    *hour = bcd2dec(data[2] & 0x3F); // 24h režim
}
