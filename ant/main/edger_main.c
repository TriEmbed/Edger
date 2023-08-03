/* HTTP Restful API Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "../components/edger_connect/include/edger_connect.h"
#include "cmd_nvs.h"
#include <tcpip_adapter.h>
#include "esp_wifi.h"

#if CONFIG_EXAMPLE_WEB_DEPLOY_SD
#include "driver/sdmmc_host.h"
#endif

#define MDNS_INSTANCE "esp ant web server"

static const char *TAG = __FILE__;
cJSON *config = NULL;

esp_err_t start_rest_server(const char *base_path);

char ip_str[17];

static void initialise_mdns(void) {
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"},
                                        {"path",  "/"}};

    ESP_ERROR_CHECK (mdns_service_add
                             ("ESP32-ANT-WebServer", "_http", "_tcp", 80, serviceTxtData, sizeof(serviceTxtData) /
                                                                                          sizeof(serviceTxtData[0])));
}

#if CONFIG_EXAMPLE_WEB_DEPLOY_SEMIHOST

esp_err_t init_fs(void) {
    esp_err_t ret = esp_vfs_semihost_register(CONFIG_EXAMPLE_WEB_MOUNT_POINT, CONFIG_EXAMPLE_HOST_PATH_TO_MOUNT);
    if (ret != ESP_OK) {
        ESP_LOGE (TAG, "Failed to register semihost driver (%s)!", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}

#endif

#if CONFIG_EXAMPLE_WEB_DEPLOY_SD
esp_err_t
init_fs (void)
{
  sdmmc_host_t host = SDMMC_HOST_DEFAULT ();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT ();

  gpio_set_pull_mode (15, GPIO_PULLUP_ONLY);	// CMD
  gpio_set_pull_mode (2, GPIO_PULLUP_ONLY);	// D0
  gpio_set_pull_mode (4, GPIO_PULLUP_ONLY);	// D1
  gpio_set_pull_mode (12, GPIO_PULLUP_ONLY);	// D2
  gpio_set_pull_mode (13, GPIO_PULLUP_ONLY);	// D3

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = true,
    .max_files = 4,
    .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t *card;
  esp_err_t ret =
    esp_vfs_fat_sdmmc_mount (CONFIG_EXAMPLE_WEB_MOUNT_POINT, &host,
                 &slot_config, &mount_config, &card);
  if (ret != ESP_OK)
    {
      if (ret == ESP_FAIL)
    {
      ESP_LOGE (TAG, "Failed to mount filesystem.");
    }
      else
    {
      ESP_LOGE (TAG, "Failed to initialize the card (%s)",
            esp_err_to_name (ret));
    }
      return ESP_FAIL;
    }
  /* print card info if mount successfully */
  sdmmc_card_print_info (stdout, card);
  return ESP_OK;
}
#endif

#if CONFIG_EXAMPLE_WEB_DEPLOY_SF
esp_err_t
init_fs (void)
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = false
  };
  esp_err_t ret = esp_vfs_spiffs_register (&conf);

  if (ret != ESP_OK)
    {
      if (ret == ESP_FAIL)
    {
      ESP_LOGE (TAG, "Failed to mount or format filesystem");
    }
      else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE (TAG, "Failed to find SPIFFS partition");
    }
      else
    {
      ESP_LOGE (TAG, "Failed to initialize SPIFFS (%s)",
            esp_err_to_name (ret));
    }
      return ESP_FAIL;
    }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info (NULL, &total, &used);
  if (ret != ESP_OK)
    {
      ESP_LOGE (TAG, "Failed to get SPIFFS partition information (%s)",
        esp_err_to_name (ret));
    }
  else
    {
      ESP_LOGI (TAG, "Partition size: total: %d, used: %d", total, used);
    }
  return ESP_OK;
}
#endif

void whichBoard() {
    config = preset_nvs();

    //Get MAC address for Ethernet
    union {
        uint8_t derived_mac_addr[6];
        uint64_t value;

    } device;
    device.value = 0;
    ESP_ERROR_CHECK (esp_read_mac(device.derived_mac_addr, ESP_MAC_ETH));
    ESP_LOGI (TAG, "Ethernet MAC 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x 0x%llx", device.derived_mac_addr[0],
              device.derived_mac_addr[1], device.derived_mac_addr[2], device.derived_mac_addr[3],
              device.derived_mac_addr[4], device.derived_mac_addr[5], device.value);
    char *deviceID = "unknown";
    switch (device.value) {

        case 0xd35260bd9e7c:
            deviceID = "white board";
            break;
        case 0xd05260bd9e7:
            deviceID = "lilyGo S2 -1";
            break;
        case 0x5bfad1c2ddbc:
            deviceID = "lyRat-mini-v1.2";
            break;
        case 0x7775bfb5aa8c:
            deviceID = "AudioKit";
            break;

    }
    cJSON_AddStringToObject(config, "device", deviceID);
    ESP_LOGI (TAG, "%s \n", (cJSON_Print(config)));
}

static int reset_cnt = 0;
// static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//     ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...ccccc %d",reset_cnt++);
//     if (reset_cnt> 10){
//       example_disconnect();
//     }
//    // esp_err_t err = esp_wifi_connect();
//    // if (err == ESP_ERR_WIFI_NOT_STARTED) {
//      //   return;
//   //  }
//   //  ESP_ERROR_CHECK(err);
// }


static void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    // esp_err_t err =ESP_OK;
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect... %d", reset_cnt++);
    if (reset_cnt > 10) {

        set_nvs_marker((int32_t) 0xa522);
        esp_restart();
    }

}

void app_main_normal(void) {
    whichBoard();
    //  ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK (esp_netif_init());
    ESP_ERROR_CHECK (esp_event_loop_create_default());
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK (edger_connect());


    ESP_ERROR_CHECK_WITHOUT_ABORT (init_fs());
    ESP_ERROR_CHECK (start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));

    tcpip_adapter_ip_info_t ipInfo;
    unsigned char *ip = (unsigned char *) &ipInfo.ip;

    // IP address.
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);


}

extern void initialize_nvs(void);

void edger_console(void);



void app_main(void) {
    initialize_nvs();
    int32_t cnt;
    get_nvs_marker(&cnt);
    printf("preset [%s]\n" ,cJSON_Print(preset_nvs()));
    ESP_LOGI (TAG, "reset cnt = 0x%x\n\n", cnt);
//    if (cnt == 0xa522) {
        edger_console();
//    }
    app_main_normal();
}