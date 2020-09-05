#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <aws_iot_shadow.h>
#include <iot_init.h>
#include <iot_error.h>
#include <iot_mqtt.h>
#include <iot_network_mbedtls.h>

#include "wifi.h"


/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_PLATFORM
    #define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_PLATFORM
#else
    #ifdef IOT_LOG_LEVEL_GLOBAL
        #define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
    #else
        #define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
    #endif
#endif

#define LIBRARY_LOG_NAME    ( "MAIN" )
#include <iot_logging_setup.h>

const char TAG[] = "main";

static void main_task(void *pArgument);

int RunShadowDemo( bool awsIotMqttMode,
                   const char * pIdentifier,
                   void * pNetworkServerInfo,
                   void * pNetworkCredentialInfo,
                   const IotNetworkInterface_t * pNetworkInterface );


void app_main(void)
{
    printf("Hello world!\n");

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize spiffs */
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };

    ret = esp_vfs_spiffs_register(&spiffs_conf);
    if (ret != ESP_OK) {
       if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        abort();
    }

    /* Initialize networking */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_station_init();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_STA_CONNECTED_BIT | WIFI_STA_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    assert((bits & WIFI_STA_CONNECTED_BIT) != 0);

    xTaskCreate(main_task, "main_task", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}


static void main_task(void *pArgument)
{
    IOT_FUNCTION_ENTRY( int, EXIT_SUCCESS );
    bool sdkInitialized = false, networkInitialized = false;
    struct IotNetworkServerInfo serverInfo = IOT_NETWORK_SERVER_INFO_MBEDTLS_INITIALIZER;
    struct IotNetworkCredentials credentials = AWS_IOT_NETWORK_CREDENTIALS_MBEDTLS_INITIALIZER;

    serverInfo.pHostName = "a17grq0o74napn-ats.iot.us-east-2.amazonaws.com";
    serverInfo.port = 443;

    credentials.pClientCert = "/spiffs/iot_cert.pem";
    credentials.pPrivateKey = "/spiffs/iot_private_key.pem";
    credentials.pRootCa = "/spiffs/iot_root_cert.pem";

    sdkInitialized = IotSdk_Init();
    if( sdkInitialized == false )
    {
        IotLogError( "IotSdk_Init failed." );
        IOT_SET_AND_GOTO_CLEANUP( EXIT_FAILURE );
    }

    networkInitialized = ( IotNetworkMbedtls_Init() == IOT_NETWORK_SUCCESS );
    if( networkInitialized == false )
    {
        IotLogError( "IotNetworkMbedtls_Init failed." );
        IOT_SET_AND_GOTO_CLEANUP( EXIT_FAILURE );
    }

    status = RunShadowDemo( false, "gtech-demo1", &serverInfo, &credentials, IOT_NETWORK_INTERFACE_MBEDTLS ); 

    IOT_FUNCTION_CLEANUP_BEGIN();

    if( networkInitialized == true )
    {
        IotNetworkMbedtls_Cleanup();
    }

    if( sdkInitialized == true )
    {
        IotSdk_Cleanup();
    }

    if( status == EXIT_SUCCESS )
    {
        IotLogInfo( "Completed successfully." );
    }

    vTaskDelete( NULL );
}
