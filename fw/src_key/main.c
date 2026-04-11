#include "includes.h"

#if __has_include("secrets.h")
    #include "secrets.h"
#else
    #include "dummy_secrets.h"
    #warning "Using default credentials. Please copy dummy_secrest.h to secrets.h and edit it to your needs"
#endif

#define millis() (to_ms_since_boot(get_absolute_time()))

#define MINMS (60*1000)
#define LOCK_REQUEST_INTERVAL (35*MINMS) // 35 min
#define LOCK_REQUEST_MINIMAL_INTERVAL 30000 // 30s
#define UNLOCK_REQUEST_MINIMAL_INTERVAL 5000 // 5s

#define COMM_BUFLEN 128
int comtx = 0, comrx = 0;
uint8_t comrx_buff[COMM_BUFLEN];
uint8_t comtx_buff[COMM_BUFLEN];

static mqtt_client_t *mqtt_client;
static uint32_t publish_counter = 0;

extern LockState lock;

/* ============================= */
/* MQTT RECEIVE CALLBACKS       */
/* ============================= */

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("MQTT RX: %.*s\n", len, (const char *)data);
    memcpy(comtx_buff, data, COMM_BUFLEN);
    comtx = len;
    LED_OFF();
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    LED_ON();
    //printf("MQTT Message on topic: %s\n", topic);
}

/* ============================= */
/* MQTT REQUEST CALLBACK         */
/* ============================= */

static void mqtt_request_cb(void *arg, err_t err) {
    if (err != ERR_OK) {
        printf("MQTT request error: %d\n", err);
    }
}

/* ============================= */
/* MQTT CONNECT CALLBACK         */
/* ============================= */

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("MQTT connected\n");

        mqtt_set_inpub_callback(client,
                                 mqtt_incoming_publish_cb,
                                 mqtt_incoming_data_cb,
                                 NULL);

        mqtt_subscribe(client,
                       MQTT_TOPIC_TO_SUBSCRIBE,
                       0,
                       mqtt_request_cb,
                       NULL);
    } else {
        printf("MQTT connection failed: %d\n", status);
        if (client) {
            mqtt_client_free(client);
            mqtt_client = NULL;
        }
    }
}

/* ============================= */
/* DNS RESOLVE CALLBACK          */
/* ============================= */

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr == NULL) {
        printf("DNS failed\n");
        return;
    }

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "pico_w_client";
    ci.client_user = MQTT_USER;
    ci.client_pass = MQTT_PASSWORD;
    ci.keep_alive = 60;

    mqtt_client = mqtt_client_new();

    mqtt_client_connect(mqtt_client,
                        ipaddr,
                        MQTT_PORT,
                        mqtt_connection_cb,
                        NULL,
                        &ci);
}

/* ============================= */
/* MAIN                          */
/* ============================= */

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    LED_INIT();
    LED_ON();

    sleep_ms(5000);

    comm_init();

    uint8_t r = 0, g = 0, b = 0;
    ws2812_init(RGB_LED_PIN);
    put_pixel(urgb_u32(r, g, b));

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_set_pulls(BUTTON_PIN, true, false);

    cyw43_arch_enable_sta_mode();

    printf("WiFi/MQTT will connect when available...\n");

    // WiFi and MQTT reconnection timing
    uint32_t wifi_check_t = millis();
    uint32_t mqtt_reconnect_t = 0;
    bool wifi_connected = false;
    bool mqtt_connected = false;

    uint32_t led_t = millis();
    uint32_t lock_last_updated_t = millis();
    uint32_t lock_request_t = millis();
    uint32_t ledw_t = millis();

    uint32_t wifi_led_t = millis();

    LED_OFF();

    while (true) {
        int32_t now = millis();
        cyw43_arch_poll();

        // WiFi status LED blinking
        if ((now - wifi_led_t) > 200) {
            static bool wifi_led_state = false;
            wifi_led_t = now;
            wifi_led_state = (!wifi_connected) /* || !mqtt_connected)*/ ? !wifi_led_state : false;
            if (wifi_led_state) {
                LED_ON();
            } else {
                LED_OFF();
            }
        }

        // ===== WiFi connection monitoring =====
        if ((now - wifi_check_t) > WIFI_CHECK_INTERVAL) {
            wifi_check_t = now;
            int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    
            // WiFi je dole
            if (link_status < CYW43_LINK_UP) { 
                if (wifi_connected) {
                    printf("WiFi connection lost!\n");
                    wifi_connected = false;
                    mqtt_connected = false;
                }
        
                printf("WiFi attempting to reconnect...\n");
                // Použijte kratší timeout, aby smyčka nezamrzla
                cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
            } 
            // WiFi je připojená
            else {
                if (!wifi_connected) {
                    printf("WiFi reconnected (IP: %s)\n", ip4addr_ntoa(netif_ip4_addr(&cyw43_state.netif[0])));
                    wifi_connected = true;
                    mqtt_reconnect_t = 0; // Zkusit MQTT hned
                }
            }
        }

        // ===== MQTT connection monitoring and reconnection =====
        if (wifi_connected && (!mqtt_client || !mqtt_client_is_connected(mqtt_client))) {
            mqtt_connected = false;
            if ((now - mqtt_reconnect_t) > MQTT_RECONNECT_INTERVAL) {
                mqtt_reconnect_t = now;
                printf("MQTT attempting to reconnect...\n");

                ip_addr_t broker_ip;
                err_t err = dns_gethostbyname(MQTT_SERVER, &broker_ip, dns_found_cb, NULL);
                
                if (err == ERR_OK) {
                    dns_found_cb(MQTT_SERVER, &broker_ip, NULL);
                    mqtt_connected = true;
                }
            }
        }

        ButtonState btn = button_poll(now);
        switch (btn) {
            case BTNST_PRESSED:
                printf("BUTTON PRESSED\n");
                break;
            case BTNST_LONG_PRESSED:
                printf("BUTTON LONG PRESSED\n");
                break;
            default:
                break;
        }

        // receive comm (from meshtastic)
        comrx = comm_poll(now, 100, comrx_buff, COMM_BUFLEN);
        if (comrx) {
            b = 255;
            comrx = strip(comrx_buff, comrx);
            printf("COMM RX: %.*s\n", COMM_BUFLEN, comrx_buff);

            if (strstr(comrx_buff, STATUS_LOCK_LOCKED) != NULL) {
                lock_last_updated_t = now;
                if (lock != LOCK_LOCKED) {
                    lock = LOCK_LOCKED;
                    r = 255;
                    g /= 2;
                    printf("LOCK LOCKED\n");
                }
            }
            else if (strstr(comrx_buff, STATUS_LOCK_UNLOCKED) != NULL) {
                lock_last_updated_t = now;
                if (lock != LOCK_UNLOCKED) {
                    lock = LOCK_UNLOCKED;
                    g = 255;
                    r /= 2;
                    printf("LOCK UNLOCKED\n");
                }
            }
        }

        // send unlock message when button pressed
        if ((btn == BTNST_PRESSED) &&
            ((now - lock_request_t) > UNLOCK_REQUEST_MINIMAL_INTERVAL) &&
            (comtx == 0) && (!comm_tx_busy()))
        {
            lock_request_t = now;
            printf("UNLOCK REQUEST\n");
            sprintf(comtx_buff, UNLOCK_REQUEST);
            comtx = strlen(comtx_buff);
        }

        // update lock status if needed
        if ((((now - lock_last_updated_t) > LOCK_REQUEST_INTERVAL) || (lock == LOCK_UNKNOWN)) &&
            ((now - lock_request_t) > LOCK_REQUEST_MINIMAL_INTERVAL) &&
            (comtx == 0) && (!comm_tx_busy()))
        {
            lock_request_t = now;
            printf("LOCK REQUEST\n");
            sprintf(comtx_buff, STATUS_LOCK_REQUEST);
            comtx = strlen(comtx_buff);
        }

        // transmitt comm (to meshtastic)
        if ((comtx>0) && (!comm_tx_busy())) {
            b = 255;
            comm_write(comtx_buff, comtx);
            printf("COMM TX: %.*s\n", comtx, comtx_buff);
            comtx = 0;
        }

        // publish what was received from meshtastic
        if (mqtt_client && mqtt_client_is_connected(mqtt_client)) {
            if (comrx) {
                LED_ON();
                printf("MQTT TX: %s\n", comrx_buff);
                mqtt_publish(mqtt_client,
                             MQTT_TOPIC_TO_PUBLISH,
                             comrx_buff,
                             strlen(comrx_buff),
                             0,
                             0,
                             mqtt_request_cb,
                             NULL);
                comrx = 0;
                LED_OFF();
            }
        }

        // some rgb status led blinking
        if ((now - led_t) > 40) {
            led_t = now;
            if (r > g) g = 0;
            if (g > r) r = 0;
            put_pixel(urgb_u32(r, g, b));
            if (r > 10) r = (int)r * 8 / 10;
            if (g > 10) g = (int)g * 8 / 10;
            if (b > 0) b = (int)b * 4 / 10;
        }

        // wifi & mqtt status blinking
        if ((now - ledw_t) > 200) {
            ledw_t = now;

            if (!wifi_connected) LED_ON();
            else if (!mqtt_connected) LED_TOGGLE();
            else LED_OFF();
        }

        //sleep_ms(10);
    }

    cyw43_arch_deinit();
    return 0;
}