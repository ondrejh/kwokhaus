#include "includes.h"

#define OVERSAMPLE 64
#define ADC_CHANNELS 3
#define BLOCK_SAMPLES (ADC_CHANNELS * OVERSAMPLE)
uint16_t adc_buffer[BLOCK_SAMPLES];
int dma_chan;

void adc_my_init(void) {
  adc_gpio_init(26); // ADC0
  adc_gpio_init(28); // ADC2
  adc_gpio_init(29); // ADC3

  // round-robin: ADC0, ADC2, ADC3
  adc_set_round_robin(
    (1 << 0) |   // ADC0
    (1 << 2) |   // ADC2
    (1 << 3)     // ADC3
  );

  // FIFO + DMA
  adc_fifo_setup(
    true,   // FIFO enable
    true,   // DMA DREQ
    1,
    false,
    false
  );

  // ADC clock ~96 kS/s
  adc_set_clkdiv(499.0f);

  // DMA channel config
  dma_chan = dma_claim_unused_channel(true);
  dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);

  dma_channel_configure(
      dma_chan,
      &cfg,
      adc_buffer,
      &adc_hw->fifo,
      BLOCK_SAMPLES,
      false
  );
}

void adc_start_once(void) {
  adc_run(true);
  dma_channel_start(dma_chan);
}

bool adc_poll(uint16_t *res) {
  if (!dma_channel_is_busy(dma_chan)) {
    uint32_t sum[ADC_CHANNELS] = {0};

    for (int i = 0; i < BLOCK_SAMPLES; i++) {
        sum[i % ADC_CHANNELS] += adc_buffer[i];
    }

    //uint16_t adc26 = sum[0] / OVERSAMPLE; // GP26 / ADC0
    //uint16_t adc28 = sum[1] / OVERSAMPLE; // GP28 / ADC2
    //uint16_t adc29 = sum[2] / OVERSAMPLE; // GP29 / ADC3
    res[0] = sum[0] / OVERSAMPLE; // GP26 / ADC0
    res[1] = sum[1] / OVERSAMPLE; // GP28 / ADC2
    res[2] = sum[2] / OVERSAMPLE; // GP29 / ADC3

    // TODO: zpracování / odeslání dat

    // znovu nastartuj DMA
    dma_channel_set_write_addr(dma_chan, adc_buffer, true);
    return true;
  }
  return false;
}

const uint16_t adc_vref = 3300; // 3.3V

uint16_t adc2u(uint16_t adc) {
  uint32_t res = adc * adc_vref * 11 / 2048;
  return res;
}

void init(void) {
  // Initialize outputs
  gpio_init(ENABLE_R_PIN);
  gpio_set_dir(ENABLE_R_PIN, GPIO_OUT);
  gpio_init(ENABLE_L_PIN);
  gpio_set_dir(ENABLE_L_PIN, GPIO_OUT);

  gpio_init(PWM_R_PIN);
  gpio_set_dir(PWM_R_PIN, GPIO_OUT);
  gpio_init(PWM_L_PIN);
  gpio_set_dir(PWM_L_PIN, GPIO_OUT);

  // Initialize nvdata (load configuration)
  load_config(&config);

  // Initialize communication
  //comm_init();

  adc_my_init();

  // Initialize onboard NeoPixel
  ws2812_init(16);
}

#define COMM_BUFLEN 128
uint8_t comm_buff[COMM_BUFLEN];

int main() {
  stdio_init_all();         // Inicializace USB CDC

  init();

  bool led = false, trig = false;
  int32_t tLed = 0, tDisp = 0, tTrig = 0, tTim = 0;
  uint8_t r = 0x00, g = 0x00, b = 0x00;

  btn_ctx_t btnL, btnR;
  button_init(&btnL, BUTTON_L_PIN);
  button_init(&btnR, BUTTON_R_PIN);

  tDisp = millis();

  adc_start_once();

  uint32_t cnt = 0;
  uint16_t adc[3];

  while (true) {
    int32_t now = millis();

    button_poll(&btnL, now);
    button_poll(&btnR, now);

    if (btnL.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      gpio_put(PWM_R_PIN, false);
      gpio_put(PWM_L_PIN, true);
      if ((r == 0) || (g != 0)) {
        g = 0;
        r = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else if (btnR.st == BTNST_HOLD) {
      gpio_put(ENABLE_L_PIN, true);
      gpio_put(ENABLE_R_PIN, true);
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, true);
      if ((g == 0) || (r != 0)) {
        r = 0;
        g = 0x10;
        put_pixel(urgb_u32(r,g,b));
      }
    }
    else {
      gpio_put(PWM_L_PIN, false);
      gpio_put(PWM_R_PIN, false);
      gpio_put(ENABLE_L_PIN, false);
      gpio_put(ENABLE_R_PIN, false);
      if ((r != 0) || (g != 0)) {
        r = g = 0;
        put_pixel(urgb_u32(r,g,b));
      }
    }

    if (adc_poll(adc)) cnt++;

    //event_t event = get_input_event(now);

    // receive comm
    /*int comrx = comm_poll(now, 100, comm_buff, COMM_BUFLEN);
    if (comrx) { // echo test
      comrx = strip(comm_buff, comrx);
      if (comrx) {
        comm_buff[comrx] = '\0';
        printf("RX: %s\n", comm_buff);

        comrx = comm_parse(comm_buff, comrx, COMM_BUFLEN);
        if (comrx) {
          comm_buff[comrx] = '\0';
          if (!comm_tx_busy()) {
            comm_write(comm_buff, comrx);
            printf("TX: %s\n", comm_buff);
          }
          else
            printf("TX BUSY\n");
        }
      }
    }*/

    // live led (green)
    if (led && ((now - tLed) >= 10)) {
      led = false;
      b = 0x00;
      put_pixel(urgb_u32(r,g,b));
    }
    if (!led && ((now - tLed) > 1000)) {
      printf("%d %d %d %d %d\n", cnt, adc[0], adc[1], adc[2], adc2u(adc[0]));
      //if (!comm_tx_busy()) {
      //  comm_write("Hello UART\r\n", 12);
      //}
      led = true;
      b = 0x10;
      put_pixel(urgb_u32(r,g,b));
      tLed = now;
    }

    /*// event notification
    switch (event) {
      case EVENT_PRESS:
        printf("Button pressed\n");
        break;
      case EVENT_LONGPRESS:
        printf("Button long pressed\n");
        break;
      case EVENT_NONE:
      default:
        break;
    }*/
  }
}
