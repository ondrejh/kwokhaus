#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();         // Inicializace USB CDC

    sleep_ms(2000);           // Krátká prodleva, aby se terminál připojil
    printf("Hello World\r\n");

    while (true) {
        sleep_ms(1000);       // případně další logika
    }
}
