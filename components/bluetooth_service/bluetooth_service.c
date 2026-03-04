#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_bt.h"
//#include "esp_bt_main.h"





#include "esp_nimble_hci.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_SERVICE";

#define SERVICE_UUID  0xFFF0
#define CHAR_UUID     0xFFF1

#define DEVICE_FLAG_UNCONFIGURED   0x00
#define DEVICE_FLAG_CONFIGURED     0x01
#define DEVICE_FLAG_CONNECTED_WIFI 0x02


uint8_t device_flag = DEVICE_FLAG_UNCONFIGURED;
static uint8_t ble_addr_type;
static int ble_gap_event(struct ble_gap_event *event, void *arg);
/* =======================
   Characteristic Handler
   ======================= */

static int characteristic_access(uint16_t conn_handle,
                                 uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt,
                                 void *arg)
{
    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_READ_CHR:
        os_mbuf_append(ctxt->om, "ESP32_DATA", strlen("ESP32_DATA"));
        return 0;

    case BLE_GATT_ACCESS_OP_WRITE_CHR: {
        char buffer[100] = {0};
        int len = OS_MBUF_PKTLEN(ctxt->om);

        if (len > sizeof(buffer) - 1)
            len = sizeof(buffer) - 1;

        memcpy(buffer, ctxt->om->om_data, len);
        buffer[len] = '\0';

        ESP_LOGI(TAG, "Received: %s", buffer);
        return 0;
    }

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

/* =======================
   GATT Service
   ======================= */

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(CHAR_UUID),
                .access_cb = characteristic_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {0}
        },
    },
    {0}
};

/* =======================
   Advertising
   ======================= */













static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* ======================
       Device Name
       ====================== */
    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    /* ======================
       Manufacturer Data
       ====================== */

    static uint8_t mfg_data[8];

    // Company ID (0xFFFF = testing)
    mfg_data[0] = 0xFF;
    mfg_data[1] = 0xFF;

    // Device status flag
    mfg_data[2] = device_flag;

    // Optional: last 4 byte MAC
//    uint8_t mac[6];
//    esp_read_mac(mac, ESP_MAC_WIFI_STA);
      uint8_t mac[6];
      esp_read_mac(mac, ESP_MAC_WIFI_STA);

    memcpy(&mfg_data[3], &mac[2], 4);

    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);

    /* ====================== */

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(
        ble_addr_type,
        NULL,
        BLE_HS_FOREVER,
        &adv_params,
        ble_gap_event,
        NULL);

    ESP_LOGI(TAG, "Advertising started");
}


static void ble_stop_advertising(void)
{
    int rc = ble_gap_adv_stop();
    ESP_LOGI(TAG, "Advertising stopped (%d)", rc);
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0)
            {
                ESP_LOGI(TAG, "Device connected");
		
            }
            else
            {
                ESP_LOGI(TAG, "Connect failed, restart advertising");
                ble_app_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Device disconnected");

	    vTaskDelay(pdMS_TO_TICKS(200));


            // ⭐ INI YANG KAMU BUTUH
            ble_app_advertise();

            break;

        default:
            break;
    }

    return 0;
}
























static void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* =======================
   Init Function
   ======================= */

void ble_service_init(void)
{
    

    /* Release Classic BT memory */
//    ESP_ERROR_CHECK(
//        esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)
//    );

    /* Init & Enable Controller */
//    esp_bt_controller_config_t bt_cfg =
//        BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    vTaskDelay(pdMS_TO_TICKS(100));




    

    // release classic BT memory (WAJIB S3)
    ESP_ERROR_CHECK(
        esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)
    );

    esp_bt_controller_config_t cfg =
        BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(
        esp_bt_controller_enable(ESP_BT_MODE_BLE)
    );





//    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
//    ESP_ERROR_CHECK(
//        esp_bt_controller_enable(ESP_BT_MODE_BLE)
//    );

    /* Init NimBLE HCI */
    ESP_ERROR_CHECK(esp_nimble_init());

    /* Init NimBLE Host */
    nimble_port_init();

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set("ESP32S3_01");

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE Initialized");
}
