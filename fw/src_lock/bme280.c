#include "includes.h"

// device has default bus address of 0x76
#define BME280_ADDR _u(0x76)

// hardware registers
#define BME280_REG_CONFIG _u(0xF5)
#define BME280_REG_CTRL_MEAS _u(0xF4)
#define BME280_REG_CTRL_HUM _u(0xF2)

#define BME280_REG_DIG_T1_LSB _u(0x88)
#define BME280_REG_DIG_H1     _u(0xA1)
#define BME280_REG_DIG_H2_LSB _u(0xE1)

#define BME280_REG_PRESSURE_MSB _u(0xF7) // Začátek datového bloku (tlak -> teplota -> vlhkost)

bool env_valid = false;
int32_t env_temp, env_press, env_humi;

struct bme280_calib_param {
    // temperature params
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    // pressure params
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;

    // --- NOVÉ: humidity params (POUZE BME280) ---
    uint8_t  dig_h1;
    int16_t  dig_h2;
    uint8_t  dig_h3;
    int16_t  dig_h4; // Pozor: v registru zabírá 12 bitů
    int16_t  dig_h5; // Pozor: v registru zabírá 12 bitů
    int8_t   dig_h6; // <-- ZMĚNA ZDE (odstraněno "signed_")

    int32_t  t_fine; // Interní proměnná nesoucí jemnou složku teploty pro tlak a vlhkost
};

// retrieve fixed compensation params
struct bme280_calib_param bme280_params;

// get callibration params - use later when recalculating raw data
bool bme280_get_calib_params(struct bme280_calib_param* params) {
    uint8_t buf_tp[24];
    uint8_t buf_h1[1];
    uint8_t buf_h2_h6[7];
    uint8_t reg;
    
    // timeout
    const uint32_t timeout = 50000;

    // T1 - T2, 24 bytes, starts 0x88
    reg = BME280_REG_DIG_T1_LSB;
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf_tp, 24, false, make_timeout_time_us(timeout)) < 0) {
        return false;
    }

    params->dig_t1 = (uint16_t)((buf_tp[1] << 8) | buf_tp[0]);
    params->dig_t2 = (int16_t)((buf_tp[3] << 8) | buf_tp[2]);
    params->dig_t3 = (int16_t)((buf_tp[5] << 8) | buf_tp[4]);

    params->dig_p1 = (uint16_t)((buf_tp[7] << 8) | buf_tp[6]);
    params->dig_p2 = (int16_t)((buf_tp[9] << 8) | buf_tp[8]);
    params->dig_p3 = (int16_t)((buf_tp[11] << 8) | buf_tp[10]);
    params->dig_p4 = (int16_t)((buf_tp[13] << 8) | buf_tp[12]);
    params->dig_p5 = (int16_t)((buf_tp[15] << 8) | buf_tp[14]);
    params->dig_p6 = (int16_t)((buf_tp[17] << 8) | buf_tp[16]);
    params->dig_p7 = (int16_t)((buf_tp[19] << 8) | buf_tp[18]);
    params->dig_p8 = (int16_t)((buf_tp[21] << 8) | buf_tp[20]);
    params->dig_p9 = (int16_t)((buf_tp[23] << 8) | buf_tp[22]);

    // H1, 1 byte, adr 0xA1
    reg = BME280_REG_DIG_H1;
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf_h1, 1, false, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    
    params->dig_h1 = buf_h1[0];

    // H2 - H6, 7 bytes, start 0xE1
    reg = BME280_REG_DIG_H2_LSB;
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf_h2_h6, 7, false, make_timeout_time_us(timeout)) < 0) {
        return false;
    }

    params->dig_h2 = (int16_t)((buf_h2_h6[1] << 8) | buf_h2_h6[0]);
    params->dig_h3 = buf_h2_h6[2];

    // concate 12bit for H4, H5
    params->dig_h4 = (int16_t)((buf_h2_h6[3] << 4) | (buf_h2_h6[4] & 0x0F));
    params->dig_h5 = (int16_t)((buf_h2_h6[5] << 4) | (buf_h2_h6[4] >> 4));
    
    params->dig_h6 = (int8_t)buf_h2_h6[6];

    return true;
}

bool bme280_init(void) {
  // I2C is "open drain", pull ups to keep signal high when no data is being sent
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  // use the "handheld device dynamic" optimal setting (see datasheet)
  uint8_t buf[2];

  const uint32_t timeout = 50000;

  // 1. idle filter (t_standby = 0.5ms BME280 / 62.5ms BMP280)
  // 0x04 << 5 t_standby, 0x05 << 2 x16 IIR filter
  const uint8_t reg_config_val = ((0x04 << 5) | (0x05 << 2)) & 0xFC;
  buf[0] = BME280_REG_CONFIG;
  buf[1] = reg_config_val;
  if (!i2c_write_blocking_until(i2c_default, BME280_ADDR, buf, 2, false, timeout)) {
    return false;
  }

  // --- 2. humidity oversampling (BME280 only) ---
  const uint8_t reg_ctrl_hum_val = 0x01; 
  buf[0] = BME280_REG_CTRL_HUM;
  buf[1] = reg_ctrl_hum_val;
  if (!i2c_write_blocking_until(i2c_default, BME280_ADDR, buf, 2, false, timeout)) {
    return false;
  }

  // 3. temperature, pressure oversampling, normal mode
  // osrs_t x1 (0x01 << 5), osrs_p x4 (0x03 << 2), normal mode operation (0x03)
  const uint8_t reg_ctrl_meas_val = (0x01 << 5) | (0x03 << 2) | (0x03);
  buf[0] = BME280_REG_CTRL_MEAS;
  buf[1] = reg_ctrl_meas_val;
  if (!i2c_write_blocking_until(i2c_default, BME280_ADDR, buf, 2, false, timeout)) {
    return false;
  }

  if (!bme280_get_calib_params(&bme280_params)) {
    return false;
  }

  return true;
}

// read raw data with timeout (return true if data read)
bool bme280_read_raw(int32_t* temp, int32_t* pressure, int32_t* humidity) {
    uint8_t buf[8]; 
    uint8_t reg = BME280_REG_PRESSURE_MSB;
    const uint32_t timeout = 50000;

    // write reg
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false; // timeout
    }
    
    // read buf
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf, 8, false, make_timeout_time_us(timeout)) < 0) {
        return false; // timeout
    }

    // pressure (20 bit z from 0, 1, 2 byte)
    *pressure = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    
    // temperature (20 bit from 3, 4, 5 byte)
    *temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
    
    // humidity (16 bit from 6, 7)
    *humidity = (buf[6] << 8) | buf[7];

    return true; // success
}

// temperature compensation, input raw temp return 100*temp (2513 ~ 25.13 °C)
int32_t bme280_convert_temp(int32_t adc_T) {
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)bme280_params.dig_t1 << 1))) * ((int32_t)bme280_params.dig_t2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bme280_params.dig_t1)) * ((adc_T >> 4) - ((int32_t)bme280_params.dig_t1))) >> 12) * ((int32_t)bme280_params.dig_t3)) >> 14;    

    bme280_params.t_fine = var1 + var2; 
    
    T = (bme280_params.t_fine * 5 + 128) >> 8;
    return T;
}

// pressure compensation (Q24.8 format)
// (pressure >> 8 to get real pressure)
int32_t bme280_convert_pressure(int32_t adc_P) {
    int32_t var1, var2;
    uint32_t p;
    
    var1 = (((int32_t)bme280_params.t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bme280_params.dig_p6);
    var2 = var2 + ((var1 * ((int32_t)bme280_params.dig_p5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)bme280_params.dig_p4) << 16);
    var1 = (((bme280_params.dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)bme280_params.dig_p2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)bme280_params.dig_p1)) >> 15);
    
    if (var1 == 0) {
        return 0; // Ochrana proti dělení nulou
    }
    
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    
    var1 = (((int32_t)bme280_params.dig_p9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)bme280_params.dig_p8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bme280_params.dig_p7) >> 4));
    
    return (int32_t)p;
}

// humidity compensation (return format Q22.10 ~ 47445 => 47445 / 1024 = 46.333 %)
int32_t bme280_convert_humidity(int32_t adc_H) {
    int32_t v_x1_u32r;

    v_x1_u32r = (bme280_params.t_fine - ((int32_t)76800));
    
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)bme280_params.dig_h4) << 20) - (((int32_t)bme280_params.dig_h5) * v_x1_u32r)) + 
                  ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)bme280_params.dig_h6)) >> 10) * 
                  (((v_x1_u32r * ((int32_t)bme280_params.dig_h3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * 
                  ((int32_t)bme280_params.dig_h2) + 8192) >> 14));
                  
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bme280_params.dig_h6)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (v_x1_u32r >> 12);
}

// data polling function (with timing)
bool env_poll(uint32_t now) {
    static uint32_t tEnv = 0;
    if ((now - tEnv) >= (ENVIRONMENT_PERIOD * 1000)) {
      tEnv = now;
      int32_t rtemp, rpress, rhum;
      if (bme280_read_raw(&rtemp, &rpress, &rhum)) {
        env_temp = bme280_convert_temp(rtemp);
        env_press = bme280_convert_pressure(rpress);
        env_humi = bme280_convert_humidity(rhum);
        env_valid = true;
        return true;
      }
      else {
        env_valid = false;
      }
    }
    return false;
}
