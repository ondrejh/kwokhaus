#include "includes.h"

void button_init(btn_ctx_t *ctx, int pin) {
  ctx->t = 0;
  ctx->cnt1 = 0;
  ctx->cnt2 = 0;
  ctx->st = BTNST_UNKNOWN;
  ctx->pin = pin;

  gpio_init(ctx->pin);
  gpio_set_dir(ctx->pin, GPIO_IN);
  gpio_set_pulls(ctx->pin, true, false);
}

ButtonState button_poll(btn_ctx_t *ctx, uint32_t now) {
  // clear one cycle states
  if ((ctx->st == BTNST_PRESSED) ||
      (ctx->st == BTNST_LONG_PRESSED)) {
    ctx->cnt1 = ctx->cnt2 = 0;
    ctx->st = BTNST_UP;
  }

  if (ctx->t != now) { // 1 ms
    ctx->t = now;
    bool press = !gpio_get(ctx->pin);

    switch (ctx->st) {

      case BTNST_UNKNOWN:
        ctx->cnt1 = ctx->cnt2 = 0;
        if (!press) ctx->st = BTNST_UP;
        break;

      case BTNST_UP:
        if (press) {
          if (ctx->cnt1++ >= BTN_MIN_PULSE) {
            ctx->st = BTNST_DOWN;
          }
        }
        else ctx->cnt1 = 0;
        break;

      case BTNST_DOWN:
        if (press) {
          ctx->cnt2 = 0;
          if (ctx->cnt1++ >= BTN_HOLD_PULSE) {
            ctx->st = BTNST_HOLD;
          }
        }
        else {
          if (ctx->cnt2++ >= BTN_MIN_PULSE) {
            ctx->st = BTNST_PRESSED;
          }
        }
        break;
    
      case BTNST_HOLD:
        if (press) ctx->cnt2 = 0;
        else {
          if (ctx->cnt2++ >= BTN_MIN_PULSE) {
            ctx->st = BTNST_LONG_PRESSED;
          }
        }
        break;
	
      default:
        ctx->cnt1 = ctx->cnt2 = 0;
        ctx->st = BTNST_UNKNOWN;
        break;
    } // switch (st)
  } // if ms

  return ctx->st;
}
