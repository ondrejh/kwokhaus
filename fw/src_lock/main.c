#include "includes.h"

#define USE_PWM_OUT

extern LockState lock;

const uint16_t adc_vref = 3300; // 3.3V

uint16_t adc2u(uint16_t adc) {
  uint16_t res = (uint32_t)(((uint32_t)adc * adc_vref * 11) / ((uint32_t)4096 * ADC_OVERSAMPLE));
  return (uint16_t)res;
}

uint32_t v2pwm(uint16_t v) {
  if (v == 0) return 0;
  if (v <= VOLT_FULL_PWR) {
    return (uint32_t)PWM_PERIOD;
  }
  uint64_t calc = (uint64_t)PWM_PERIOD * VOLT_FULL_PWR;
  return (uint32_t)(calc / v);
}

bool adc_poll(uint32_t now, uint16_t *adc) {
  static uint32_t tAdc = 0;
  static int cnt = 0;
  static uint16_t a = 0;
  if ((now - tAdc) >= ADC_POLL_PERIOD) {
    tAdc = now;
    adc_select_input(SENSE_VIN_ADC);
    a += adc_read();
    cnt ++;
    if (cnt >= ADC_OVERSAMPLE) {
      *adc = a;
      a = 0;
      cnt = 0;
      return true;
    }
  }
  return false;
}

#define REG_RESET _u(0xE0)

#define REG_TEMP_XLSB _u(0xFC)
#define REG_TEMP_LSB _u(0xFB)
#define REG_TEMP_MSB _u(0xFA)

#define REG_PRESSURE_XLSB _u(0xF9)
#define REG_PRESSURE_LSB _u(0xF8)
#define REG_PRESSURE_MSB _u(0xF7)

// calibration registers
#define REG_DIG_T1_LSB _u(0x88)
#define REG_DIG_T1_MSB _u(0x89)
#define REG_DIG_T2_LSB _u(0x8A)
#define REG_DIG_T2_MSB _u(0x8B)
#define REG_DIG_T3_LSB _u(0x8C)
#define REG_DIG_T3_MSB _u(0x8D)
#define REG_DIG_P1_LSB _u(0x8E)
#define REG_DIG_P1_MSB _u(0x8F)
#define REG_DIG_P2_LSB _u(0x90)
#define REG_DIG_P2_MSB _u(0x91)
#define REG_DIG_P3_LSB _u(0x92)
#define REG_DIG_P3_MSB _u(0x93)
#define REG_DIG_P4_LSB _u(0x94)
#define REG_DIG_P4_MSB _u(0x95)
#define REG_DIG_P5_LSB _u(0x96)
#define REG_DIG_P5_MSB _u(0x97)
#define REG_DIG_P6_LSB _u(0x98)
#define REG_DIG_P6_MSB _u(0x99)
#define REG_DIG_P7_LSB _u(0x9A)
#define REG_DIG_P7_MSB _u(0x9B)
#define REG_DIG_P8_LSB _u(0x9C)
#define REG_DIG_P8_MSB _u(0x9D)
#define REG_DIG_P9_LSB _u(0x9E)
#define REG_DIG_P9_MSB _u(0x9F)

// number of calibration registers to be read
#define NUM_CALIB_PARAMS 24

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

// Funkce nově vrací true (úspěch) nebo false (chyba/timeout)
bool bme280_get_calib_params(struct bme280_calib_param* params) {
    uint8_t buf_tp[24];
    uint8_t buf_h1[1];
    uint8_t buf_h2_h6[7];
    uint8_t reg;
    
    // Časový limit pro každou I2C operaci (50 000 mikrosekund = 50 ms)
    const uint32_t timeout = 50000;

    // =================================================================
    // 1. ČTENÍ: Teplota a Tlak (T1 až P9) - 24 bajtů z adresy 0x88
    // =================================================================
    reg = REG_DIG_T1_LSB;
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

    // =================================================================
    // 2. ČTENÍ: Vlhkost H1 - 1 bajt z adresy 0xA1
    // =================================================================
    reg = BME280_REG_DIG_H1;
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf_h1, 1, false, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    
    params->dig_h1 = buf_h1[0];

    // =================================================================
    // 3. ČTENÍ: Vlhkost H2 až H6 - 7 bajtů z adresy 0xE1
    // =================================================================
    reg = BME280_REG_DIG_H2_LSB;
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false;
    }
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf_h2_h6, 7, false, make_timeout_time_us(timeout)) < 0) {
        return false;
    }

    params->dig_h2 = (int16_t)((buf_h2_h6[1] << 8) | buf_h2_h6[0]);
    params->dig_h3 = buf_h2_h6[2];

    // Skládání 12-bitových hodnot pro H4 a H5
    params->dig_h4 = (int16_t)((buf_h2_h6[3] << 4) | (buf_h2_h6[4] & 0x0F));
    params->dig_h5 = (int16_t)((buf_h2_h6[5] << 4) | (buf_h2_h6[4] >> 4));
    
    //params->signed_dig_h6 = (int8_t)buf_h2_h6[6];
    params->dig_h6 = (int8_t)buf_h2_h6[6];

    return true; // Všechny bloky byly úspěšně načteny
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

  // 1. Nastavení filtru a doby nečinnosti (t_standby = 0.5ms pro BME280 / 62.5ms pro BMP280 podle revize)
  // 0x04 << 5 nastaveno pro t_standby, 0x05 << 2 pro x16 IIR filtr
  const uint8_t reg_config_val = ((0x04 << 5) | (0x05 << 2)) & 0xFC;
  buf[0] = BME280_REG_CONFIG;
  buf[1] = reg_config_val;
  if (!i2c_write_blocking_until(i2c_default, BME280_ADDR, buf, 2, false, timeout)) {
    return false;
  }

  // --- 2. NOVÉ: NASTAVENÍ OVERSAMPLINGU PRO VLHKOST (POUZE BME280) ---
  // Pro "handheld device dynamic" doporučuje datasheet osrs_h = x1 (0x01)
  const uint8_t reg_ctrl_hum_val = 0x01; 
  buf[0] = BME280_REG_CTRL_HUM;
  buf[1] = reg_ctrl_hum_val;
  if (!i2c_write_blocking_until(i2c_default, BME280_ADDR, buf, 2, false, timeout)) {
    return false;
  }

  // 3. Nastavení oversamplingu teploty, tlaku a spuštění Normal módu
  // osrs_t x1 (0x01 << 5), osrs_p x4 (0x03 << 2), normal mode operation (0x03)
  // POZOR: Tento zápis v sobě nese i potvrzení změn v registru CTRL_HUM provedených výše!
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
// Funkce nově vrací true (úspěch) nebo false (chyba/timeout)
bool bme280_read_raw(int32_t* temp, int32_t* pressure, int32_t* humidity) {
    // Potřebujeme 8 bajtů: 3 pro tlak, 3 pro teplotu, 2 pro vlhkost
    uint8_t buf[8]; 
    uint8_t reg = BME280_REG_PRESSURE_MSB;
    
    // Časový limit pro I2C operace (50 ms)
    const uint32_t timeout = 50000;

    // Zápis adresy počátečního registru s timeoutem
    if (i2c_write_blocking_until(i2c_default, BME280_ADDR, &reg, 1, true, make_timeout_time_us(timeout)) < 0) {
        return false; // Timeout nebo chyba sběrnice
    }
    
    // Načtení 8 bajtů v jednom kuse s timeoutem
    if (i2c_read_blocking_until(i2c_default, BME280_ADDR, buf, 8, false, make_timeout_time_us(timeout)) < 0) {
        return false; // Timeout nebo chyba sběrnice
    }

    // Tlak (20 bitů z bajtů 0, 1, 2)
    *pressure = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    
    // Teplota (20 bitů z bajtů 3, 4, 5)
    *temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
    
    // --- NOVÉ: Vlhkost (16 bitů z bajtů 6 a 7) ---
    // Na rozdíl od tlaku a teploty je vlhkost uložena standardně jako čisté 16bitové číslo (MSB a LSB)
    *humidity = (buf[6] << 8) | buf[7];

    return true; // Data úspěšně přečtena
}

// 1. KOMPENZACE TEPLOTY
// Vrací teplotu v setinách stupně Celsia (např. 2513 znamená 25.13 °C)
/*int32_t bme280_convert_temp(int32_t adc_T, struct bme280_calib_param* params) {
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)params->dig_t1 << 1))) * ((int32_t)params->dig_t2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)params->dig_t1)) * ((adc_T >> 4) - ((int32_t)params->dig_t1))) >> 12) * ((int32_t)params->dig_t3)) >> 14;
    
    // Uložení t_fine do struktury - kritické pro následný tlak a vlhkost!
    params->t_fine = var1 + var2; 
    
    T = (params->t_fine * 5 + 128) >> 8;
    return T;
}*/

int32_t bme280_convert_temp(int32_t adc_T, struct bme280_calib_param* params) {
    int32_t var1, var2, T;
    
    // Výpočet jemné složky teploty
    var1 = ((((adc_T >> 3) - ((int32_t)params->dig_t1 << 1))) * ((int32_t)params->dig_t2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)params->dig_t1)) * ((adc_T >> 4) - ((int32_t)params->dig_t1))) >> 12) * ((int32_t)params->dig_t3)) >> 14;
    
    // Kritické pro tlak a vlhkost!
    params->t_fine = var1 + var2; 
    
    // Výsledná teplota v setinách stupně Celsia
    T = (params->t_fine * 5 + 128) >> 8;
    return T;
}

// 2. KOMPENZACE TLAKU
// Vrací tlak v Pascalech jako beznaménkové 32bitové číslo ve formátu Q24.8 
// (pro získání běžných Pascalů stačí výsledek posunout o 8 doprava: pressure >> 8)
int32_t bme280_convert_pressure(int32_t adc_P, struct bme280_calib_param* params) {
    int32_t var1, var2;
    uint32_t p;
    
    var1 = (((int32_t)params->t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)params->dig_p6);
    var2 = var2 + ((var1 * ((int32_t)params->dig_p5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)params->dig_p4) << 16);
    var1 = (((params->dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)params->dig_p2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)params->dig_p1)) >> 15);
    
    if (var1 == 0) {
        return 0; // Ochrana proti dělení nulou
    }
    
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    
    var1 = (((int32_t)params->dig_p9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)params->dig_p8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + params->dig_p7) >> 4));
    
    return (int32_t)p;
}

// 3. KOMPENZACE VLHKOSTI (Pouze BME280)
// Vrací relativní vlhkost v % ve formátu Q22.10 (např. 47445 znamená 47445 / 1024 = 46.333 %)
int32_t bme280_convert_humidity(int32_t adc_H, struct bme280_calib_param* params) {
    int32_t v_x1_u32r;

    v_x1_u32r = (params->t_fine - ((int32_t)76800));
    
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)params->dig_h4) << 20) - (((int32_t)params->dig_h5) * v_x1_u32r)) + 
                  ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)params->dig_h6)) >> 10) * 
                  (((v_x1_u32r * ((int32_t)params->dig_h3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * 
                  ((int32_t)params->dig_h2) + 8192) >> 14));
                  
    //v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)params->signed_dig_h6)) >> 4));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)params->dig_h6)) >> 4));
    // Ošetření podtečení a přetečení (vlhkost nemůže být menší než 0 % a větší než 100 %)
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (v_x1_u32r >> 12);
}

void init(void) {
  // Initialize outputs
  // LED
  gpio_init(LED_GREEN_PIN);
  gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
  // Unlock trigger
  gpio_init(TRIGGER_PIN);
  gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
  // Button
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_pulls(BUTTON_PIN, true, false);
  // Lock state input
  gpio_init(LOCK_PIN);
  gpio_set_dir(LOCK_PIN, GPIO_IN);
  gpio_set_pulls(LOCK_PIN, true, false);
  // PIR sensor input
  gpio_init(PIR_INPUT_PIN);
  gpio_set_dir(PIR_INPUT_PIN, GPIO_IN);
  gpio_set_pulls(PIR_INPUT_PIN, true, false);

#ifdef USE_PWM_OUT
  // Initialize PWM
  gpio_set_function(TRIGGER_PIN, GPIO_FUNC_PWM);
  uint slTrig = pwm_gpio_to_slice_num(TRIGGER_PIN);
  pwm_set_wrap(slTrig, PWM_PERIOD);
  pwm_set_gpio_level(TRIGGER_PIN, 0);
  pwm_set_enabled(slTrig, true);
#endif

    // initialize ADC
  adc_init();
  adc_gpio_init(SENSE_VIN_PIN);

  // Initialize communication
  comm_init();

  // Initialize onboard NeoPixel
  led_rgb_onboard_init(16);
  led_onboard(urgb_u32(0, 0, 0));

  // Initialize RGB Light
  rgb_light_init(3);
  rgb_light(urgb_u32(0, 0, 0));

  if (!bme280_init()) {
    printf("BME280 not connected!\n");
  }
}

#define COMM_BUFLEN 128
uint8_t comm_buff[COMM_BUFLEN];

uint32_t light_power_to_rgb(uint16_t pwr) {
  if (pwr <= 0)
    return urgb_u32(0, 0, 0);
  if (pwr >= LIGHT_PWR_MAX)
    return urgb_u32(255, 255, 255);
  uint8_t p = pwr * 255 / LIGHT_PWR_MAX;
  return urgb_u32(p, p, p);
}

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false;
  int32_t tLed = 0;

  bool trig = false;
  int32_t tTrig = 0, tLastTx = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00; 

  int16_t adcVin = 0;
  int32_t tAdcNot = 0;

  uint32_t pwm = 0;
  uint16_t voltage = 0;

  bool light = false;
  uint16_t lightPwr = 0;
  uint32_t tLight = 0;
  uint32_t lightChng = LIGHT_CHANGE_SLOW;
  uint32_t tLightOff = 0;
  uint32_t lightOffTout = 0;

  uint32_t tBme280 = 0;

  while (true) {
    int32_t now = millis();
    bool trig_now = false;

    // grab events
    event_t event = get_input_event(now); // input events

    // receive comm
    int comrx = comm_poll(now, 100, comm_buff, COMM_BUFLEN);
    if (comrx) { // echo test
      comrx = strip(comm_buff, comrx);
      if (comrx) {
        printf("RX: %s\n", comm_buff);

        comrx = comm_parse(comm_buff, comrx, COMM_BUFLEN, &event);
        if (comrx) {
          comm_buff[comrx] = '\0';
          if (!comm_tx_busy()) {
            tLastTx = now;
            comm_write(comm_buff, comrx);
            printf("TX: %s\n", comm_buff);
          }
          else
            printf("TX BUSY\n");
        }
      }
    }

    if ((!comm_tx_busy()) && ((now - tLastTx) > STATUS_REPEAT_PERIOD)) {
      tLastTx = now;
      comrx = sprint_status(comm_buff, COMM_BUFLEN);
      comm_write(comm_buff, comrx);
      printf("TX: %s\n", comm_buff);
    }

  #ifdef LIFE_LED
    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      led_onboard(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 2000)) {
      led = true;
      b = 0x10;
      led_onboard(urgb_u32(r,g,b));
      tLed = now;
    }
  #endif

    // event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        light = !light;
        lightChng = LIGHT_CHANGE_FAST;
        lightOffTout = light ? LIGHT_OFF_TIMEOUT : 0;
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_LOCK:
        lightOffTout = LIGHT_OFF_TIMEOUT;
        lightChng = LIGHT_CHANGE_SLOW;
      case EVENT_UNLOCK:
        printf("Lock %s\n", (event==EVENT_LOCK)?"locked":"unlocked");
        comrx = sprint_status(comm_buff, COMM_BUFLEN);
        comm_write(comm_buff, comrx);
        tLastTx = now;
        break;
      case EVENT_CMD_UNLOCK:
        printf("Unlock command received\n");
        trig_now = true;
        break;
      case EVENT_CMD_LIGHT:
        printf("Light ON command received\n");
        light = true;
        if (lock==LOCK_LOCKED) {
          lightOffTout = LIGHT_OFF_TIMEOUT;
        }
        break;
      case EVENT_FREE:
        printf("Kwokhaus is free\n");
        gpio_put(LED_GREEN_PIN, false);
        break;
      case EVENT_OCUPY:
        printf("Kwokhaus ocupied\n");
        gpio_put(LED_GREEN_PIN, true);
        break;
      case EVENT_NONE:
      default:
        break;
    }

    // measure input voltage, get current pwm output (12V out)
    if (adc_poll(now, &adcVin)) {
      voltage = adc2u(adcVin);
      pwm = v2pwm(voltage);
#ifdef USE_PWM_OUT
      pwm_set_gpio_level(TRIGGER_PIN, trig ? pwm : 0);
#endif
    }
    /*// print it (debug)
    if ((now - tAdcNot) > 2000) {
      tAdcNot = now;
      printf("%0.01fV %d/%d DC\n", (float)voltage/1000.0, PWM_PERIOD, pwm);
    }*/

    // trigger
    if (trig && ((now - tTrig) >= 500)) {
      trig = false;
#ifndef USE_PWM_OUT
      gpio_put(TRIGGER_PIN, trig);
#endif
      r = 0;
      led_onboard(urgb_u32(r,g,b));
    }
    if (!trig && trig_now) {
      tTrig = now;
      trig = true;
#ifndef USE_PWM_OUT
      gpio_put(TRIGGER_PIN, trig);
#endif
      r = 0x80;
      led_onboard(urgb_u32(r,g,b));
    }

    // light off timeout
    if (light & (lightOffTout != 0)) {
      if ((now - tLightOff) >= (lightOffTout * 1000)) {
        light = false;
        lightOffTout = 0;
      }
    }
    else {
      tLightOff = now;
    }

    // light shade in/out
    if ((light && (lightPwr<LIGHT_PWR_MAX)) || (!light && (lightPwr > 0))) {
      if ((now - tLight) >= lightChng) {
        tLight = now;
        lightPwr += light ? 1 : -1;
        rgb_light(light_power_to_rgb(lightPwr));
      }
    } 
    else {
      tLight = now;
    }

    if ((now - tBme280) > 5000) {
      tBme280 = now;
      int32_t rtemp, rpress, rhum;
      if (bme280_read_raw(&rtemp, &rpress, &rhum)) {
        //printf("Kalibrace T1: %d, T2: %d, T3: %d\n", bme280_params.dig_t1, bme280_params.dig_t2, bme280_params.dig_t3);
        //printf("raw temp 0x%X,  press 0x%X,  humi 0x%X\n", rtemp, rpress, rhum);
        printf("Temp.    = %.1f C\n", bme280_convert_temp(rtemp, &bme280_params) / 100.f);
        printf("Pressure = %.2f kPa\n", bme280_convert_pressure(rpress, &bme280_params) / 1000.f);
        printf("Humidity = %.0f %%\n", bme280_convert_humidity(rhum, &bme280_params) / 1024.f);
      }
    }

    //gpio_put(LED_GREEN_PIN, comm_tx_busy());
  }
}
