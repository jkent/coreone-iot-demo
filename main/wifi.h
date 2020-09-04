#pragma once

#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_STA_CONNECTING_BIT     BIT0
#define WIFI_STA_CONNECTED_BIT      BIT1
#define WIFI_STA_DISCONNECTED_BIT   BIT2
#define WIFI_STA_FAIL_BIT           BIT3

EventGroupHandle_t wifi_event_group;
esp_netif_t *wifi_sta_netif;

void wifi_station_init(void);
