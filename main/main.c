
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"

//BLE
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/util/util.h"

#include "console/console.h"

#include "services/gap/ble_svc_gap.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "phy_prph.h"

#define TIMER   1

static const char *tag = "ESP32";

static TimerHandle_t ble_rx_timer;

//task
static bool notify_state;
static SemaphoreHandle_t    notify_mutex;
static TaskHandle_t tskHandle;

static uint16_t conn_handle;

static const char *device_name = "ESP32BOARD";

static int ble_gap_event(struct ble_gap_event *event, void *arg);

static uint8_t blehr_addr_type;

static uint8_t  dummyData = 90;

/**
 * Utility function to log an array of bytes.
 */
void    print_bytes(const uint8_t *bytes, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        MODLOG_DFLT(INFO, "%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

void    print_addr(const void *addr)
{
    const uint8_t *u8p;

    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}


/*
 * Enables advertising with parameters:
 *     o General discoverable mode
 *     o Undirected connectable mode
 */
static void ble_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[])
    {
        service_uuid,
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(blehr_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static void ble_rx_loop_stop(void)
{
    xTimerStop( ble_rx_timer, 1000 / portTICK_PERIOD_MS );
}

/* Reset */
static void ble_rx_loop_reset(void)
{
    int rc;

    if (xTimerReset(ble_rx_timer, 1000 / portTICK_PERIOD_MS ) == pdPASS) {
        rc = 0;
    } else {
        rc = 1;
    }

    assert(rc == 0);

}

/* This function run loop and notifies it to the client */
#if TIMER
static void ble_rx_loop(TimerHandle_t ev)
{
    static uint8_t hrm[2];
    static int rc;
    static struct os_mbuf *om;

    if (!notify_state) {
        ble_rx_loop_stop();
        dummyData = 90;
        return;
    }

    hrm[0] = 0x06; /* dummy eader */
    hrm[1] = dummyData; /* storing dummy data */

    /* Simulation of heart beats */
    dummyData++;
    if (dummyData == 160) {
        dummyData = 90;
    }

    om = ble_hs_mbuf_from_flat(hrm, sizeof(hrm));
    rc = ble_gatts_notify_custom(conn_handle, rx_handle, om);

    assert(rc == 0);

    ble_rx_loop_reset();
}
#else
static void ble_rx_loop(void *param)
{
    static uint8_t  data[2];
    int  rc;
    struct os_mbuf *om;

    while (1)
    {
        xSemaphoreTake(notify_mutex, 1000 / portTICK_PERIOD_MS);
        if (!notify_state)
        {
            xSemaphoreGive(notify_mutex);
            continue;
        }
        xSemaphoreGive(notify_mutex);
        
        data[0] = 0x06; /* contact of a sensor */
        data[1] = heartrate; /* storing dummy data */
        /* Simulation of heart beats */
        heartrate++;
        if (heartrate == 160)
        {
            heartrate = 90;
        }
        om = ble_hs_mbuf_from_flat(data, sizeof(data));
        rc = ble_gatts_notify_custom(conn_handle, rx_handle, om);
        assert(rc == 0);
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    MODLOG_DFLT(INFO, "rx loop End.");
    vTaskDelete(NULL);
}
#endif

static int  ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed */
        MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising */
            ble_advertise();
        }
        conn_handle = event->connect.conn_handle;
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);

        /* Connection terminated; resume advertising */
        ble_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "adv complete\n");
        ble_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        MODLOG_DFLT(INFO, "subscribe event; cur_notify=%d\n value handle; "
                    "val_handle=%d\n",
                    event->subscribe.cur_notify, rx_handle);
        if (event->subscribe.attr_handle == rx_handle)
        {
            #if TIMER
            notify_state = event->subscribe.cur_notify;
            ble_rx_loop_reset(); //timer activate
            #else
            xSemaphoreTake(notify_mutex, 1000 / portTICK_PERIOD_MS);
            notify_state = event->subscribe.cur_notify;
            xSemaphoreGive(notify_mutex);
            #endif
        }
        else if (event->subscribe.attr_handle != rx_handle)
        {
            #if TIMER
            ble_rx_loop_stop(); //timer deactivate
            #else
            xSemaphoreTake(notify_mutex, 1000 / portTICK_PERIOD_MS);
            notify_state = event->subscribe.cur_notify;
            xSemaphoreGive(notify_mutex);
            #endif
        }
        ESP_LOGI("BLE_GAP_SUBSCRIBE_EVENT", "conn_handle from subscribe=%d", conn_handle);
        break;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.value);
        break;

    }

    return 0;
}

static void
ble_on_sync(void)
{
    int rc;

    rc = ble_hs_id_infer_auto(0, &blehr_addr_type);
    assert(rc == 0);

    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(blehr_addr_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");

    /* Begin advertising */
    ble_advertise();
}

static void
ble_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

void ble_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
    vSemaphoreDelete(notify_mutex);
    vTaskDelete(tskHandle);
}

void app_main(void)
{
    int rc;

    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        MODLOG_DFLT(ERROR, "Failed to init nimble %d \n", ret);
        return;
    }

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    /* name, period/time,  auto reload, timer ID, callback */
    /* task start */
    notify_state = 0;
    notify_mutex = xSemaphoreCreateMutex();
    #if TIMER
    ble_rx_timer = xTimerCreate("blehr_tx_timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, ble_rx_loop);
    #else
    xTaskCreate(ble_rx_loop, "ble_rx_loop", 2048, NULL, tskIDLE_PRIORITY, &tskHandle);
    #endif

    rc = gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);

    /* Start the task */
    nimble_port_freertos_init(ble_host_task);

}
