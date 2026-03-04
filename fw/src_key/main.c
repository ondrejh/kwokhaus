#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#if __has_include("secrets.h")
    #include "secrets.h"
#else
    #include "dummy_secrets.h"
    #warning "Using default credentials. Please copy dummy_secrest.h to secrets.h and edit it to your needs"
#endif

//#define MQTT_PORT 1883
#define PUBLISH_INTERVAL_MS 10000

static mqtt_client_t *mqtt_client;
static uint32_t publish_counter = 0;

/* ============================= */
/* MQTT RECEIVE CALLBACKS       */
/* ============================= */

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("MQTT RX: %.*s\n", len, (const char *)data);
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    printf("MQTT Message on topic: %s\n", topic);
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
    sleep_ms(2000);

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
        cyw43_arch_poll();

        if (mqtt_client && mqtt_client_is_connected(mqtt_client)) {
            if (absolute_time_diff_us(get_absolute_time(), next_publish) < 0) {

                char payload[64];
                snprintf(payload, sizeof(payload),
                         "{\"cnt\": %lu}", publish_counter++);

                mqtt_publish(mqtt_client,
                             MQTT_TOPIC_TO_PUBLISH,
                             payload,
                             strlen(payload),
                             0,
                             0,
                             mqtt_request_cb,
                             NULL);

                next_publish = make_timeout_time_ms(PUBLISH_INTERVAL_MS);
            }
        }

        sleep_ms(10);
    }

    cyw43_arch_deinit();
    return 0;
}