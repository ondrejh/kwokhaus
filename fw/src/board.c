#include "includes.h"


ButtonState button_poll(uint32_t now) {
  static uint32_t t = 0;
  static uint32_t cnt = 0;
  static ButtonState s = BTNST_UNKNOWN;

  if (t != now) {
    bool press = !gpio_get(BUTTON_PIN);

    switch (ButtonState) {
      case BTNST_UNKNOWN:
        cnt = 0;
        if (!press) ButtonState = BTNST_UP;
        break;
      case BTNST_UP:
        if (press) {
          if (cnt++ >= BTN_MIN_PULSE) {
            ButtonState = BTNST_DOWN;
            cnt = 0;
          }
        }
        else cnt = 0;
        break;
      case BTNST_DOWN:
        if (!press) {
          if (cnt++ >= BTN_MIN_PULSE) {
            
          }
        }
        else cnt = 0;
        break;

    
    
  }

}
