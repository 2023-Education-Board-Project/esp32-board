#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble_server.h"
#include "lvgl_ui.h"

static const char *model_num = "ESP32-BOARD";
static const char *serial_num = "MAC_ADRESS";
static const char *firmware_rev = "unknown";

uint16_t rx_handle;
uint16_t tx_handle;


ble_uuid16_t    deviceInfo_uuid = BLE_UUID16_INIT(0x180A);
ble_uuid16_t    modelNum_uuid = BLE_UUID16_INIT(0x2a24);
ble_uuid16_t    serialNum_uuid = BLE_UUID16_INIT(0x2a25);
ble_uuid16_t    firmwareRevision_uuid = BLE_UUID16_INIT(0x2a26);

ble_uuid16_t    service_uuid = BLE_UUID16_INIT(0xf005);
ble_uuid128_t   rxChar_uuid = BLE_UUID128_INIT(0xcc,0x97,0x00,0x22,0x80,0x7c,0x0b,0x85,
                                            0xab,0x42,0x7e,0xfa,0x01,0xda,0x61,0x52);
ble_uuid128_t   txChar_uuid = BLE_UUID128_INIT(0xcc,0x97,0x00,0x22,0x80,0x7c,0x0b,0x85,
                                            0xab,0x42,0x7e,0xfa,0x02,0xda,0x61,0x52);

static int  read(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);
static int  write(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static int  modelNum_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int  serialNum_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static int  firmwareRev_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Custom */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[])
        {
            {
                /* Characteristic: read */
                .uuid = &rxChar_uuid.u,
                .access_cb = read,
                .val_handle = &rx_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                /* Characteristic: write */
                .uuid = &txChar_uuid.u,
                .access_cb = write,
                .val_handle = &tx_handle,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        /* Service: Device Information */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &deviceInfo_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[])
        {
            {
                /* Characteristic: * Model number string */
                .uuid = &modelNum_uuid.u,
                .access_cb = modelNum_info,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                /* Characteristic: Serial number string */
                .uuid = &serialNum_uuid.u,
                .access_cb = serialNum_info,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                /* Characteristic: Firmware revision string */
                .uuid = &firmwareRevision_uuid.u,
                .access_cb = firmwareRev_info,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                0, /* No more characteristics in this service */
            },
        }
    },
    {
        0, /* No more services */
    },
};

static int  read(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    /* Sensor location, set to "Chest" */
    static uint8_t body_sens_loc = 0x01;
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    rc = os_mbuf_append(ctxt->om, &body_sens_loc, sizeof(body_sens_loc));

    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int  write(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t func;
    char    *data;
    
    MODLOG_DFLT(INFO, "Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    
    //parsing
    func = ((uint8_t *)ctxt->om->om_data)[0];
    data = heap_caps_malloc(sizeof(char) * ctxt->om->om_len, MALLOC_CAP_8BIT);
    assert(data != 0);

    for (int i = 0; i < ctxt->om->om_len - 1; i++)
    {
        data[i] = ((uint8_t *)ctxt->om->om_data)[i + 1];
    }
    data[ctxt->om->om_len - 1] = '\0';
    
    //call func
    switch(func)
    {
        case 0x81:
            print_text_lcd("%s\n", data);
            heap_caps_free(data);
            break;
        default:
            ;
    }
    return 0;
}

static int  modelNum_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    
    rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int  serialNum_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    
    rc = os_mbuf_append(ctxt->om, serial_num, strlen(serial_num));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int  firmwareRev_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    
    rc = os_mbuf_append(ctxt->om, firmware_rev, strlen(firmware_rev));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

void    gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
