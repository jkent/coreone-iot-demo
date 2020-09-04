#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "wifi.h"

static const char TAG[] = "wifi";

esp_netif_t *wifi_sta_netif;
EventGroupHandle_t wifi_event_group;


static void event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xEventGroupClearBits(wifi_event_group, WIFI_STA_FAIL_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECTING_BIT);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_STA_CONNECTED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_STA_DISCONNECTED_BIT);
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        if (event->reason != WIFI_REASON_ASSOC_LEAVE) {
            xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECTING_BIT | WIFI_STA_FAIL_BIT);
            ESP_LOGI(TAG, "Failed to connect to AP. Retrying.");
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_STA_FAIL_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP address acquired: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupClearBits(wifi_event_group, WIFI_STA_CONNECTING_BIT | WIFI_STA_DISCONNECTED_BIT | WIFI_STA_FAIL_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECTED_BIT);
    }
}


void wifi_station_init(void)
{
    wifi_event_group = xEventGroupCreate();

    wifi_sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
            IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = { };
    //ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
    strncpy((char*)&wifi_config.sta.ssid, "Kent-guest", 32);
    strncpy((char*)&wifi_config.sta.password, "leechthis", 64);
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi station init complete.");
}
