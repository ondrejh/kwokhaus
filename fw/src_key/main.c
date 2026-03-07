#include "includes.h"

#if __has_include("secrets.h")
    #include "secrets.h"
#else
    #include "dummy_secrets.h"
    #warning "Using default credentials. Please copy dummy_secrest.h to secrets.h and edit it to your needs"
#endif

#define millis() (to_ms_since_boot(get_absolute_time()))

//#define MQTT_PORT 1883
#define PUBLISH_INTERVAL_MS 10000

#define COMM_BUFLEN 128
int comtx = 0, comrx = 0;
uint8_t comrx_buff[COMM_BUFLEN];
uint8_t comtx_buff[COMM_BUFLEN];

static mqtt_client_t *mqtt_client;
static uint32_t publish_counter = 0;

/* ============================= */
/* MQTT RECEIVE CALLBACKS       */
/* ============================= */

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("MQTT RX: %.*s\n", len, (const char *)data);
    memcpy(comtx_buff, data, COMM_BUFLEN);
    comtx = len;
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
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

    comm_init();
    //sleep_ms(2000);

    ws2812_init(RGB_LED_PIN);
    put_pixel(urgb_u32(0,0,10));

    LED_INIT();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            30000)) {

        printf("WiFi failed\n");
        return -1;
    }

    printf("WiFi connected\n");

    ip_addr_t broker_ip;
    err_t err = dns_gethostbyname(MQTT_SERVER,
                                  &broker_ip,
                                  dns_found_cb,
                                  NULL);

    if (err == ERR_OK) {
        dns_found_cb(MQTT_SERVER, &broker_ip, NULL);
    }

    absolute_time_t next_publish = make_timeout_time_ms(PUBLISH_INTERVAL_MS);

    while (true) {
        int32_t now = millis();
        cyw43_arch_poll();

        // receive comm
        comrx = comm_poll(now, 100, comrx_buff, COMM_BUFLEN);
        if (comrx) {
            //strip(comrx_buff, comrx);
            printf("COMM RX: %.*s\n", COMM_BUFLEN, comrx_buff);
        }

        // transmitt comm
        if ((comtx>0) && (!comm_tx_busy())) {
            //strip(comtx_buff, comtx);
            comm_write(comtx_buff, comtx);
            printf("COMM TX: %.*s\n", comtx, comtx_buff);
            comtx = 0;
        }

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

                next_publish = make_timeout_time_ms(PUBLISH_INTERVAL_MS);
                comrx = 0;
                LED_OFF();
            }
            if (absolute_time_diff_us(get_absolute_time(), next_publish) < 0) {
                LED_ON();
                char payload[64];
                snprintf(payload, sizeof(payload),
                         "{\"cnt\": %lu}", publish_counter++);

                //printf("MQTT Message on topic: %s\n", MQTT_TOPIC_TO_PUBLISH);
                printf("MQTT TX: %s\n", payload);

                mqtt_publish(mqtt_client,
                             MQTT_TOPIC_TO_PUBLISH,
                             payload,
                             strlen(payload),
                             0,
                             0,
                             mqtt_request_cb,
                             NULL);

                next_publish = make_timeout_time_ms(PUBLISH_INTERVAL_MS);
                LED_OFF();
            }
        }

        sleep_ms(10);
    }

    cyw43_arch_deinit();
    return 0;
}