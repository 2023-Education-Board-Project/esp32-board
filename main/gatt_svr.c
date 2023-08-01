/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "phy_prph.h"

static uint8_t *le_phy_val;
static uint16_t gatt_svr_chr_val_handle;

static uint16_t read_val_handle;
static uint16_t write_val_handle;

static int
gatt_svr_chr_access_le_phy(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt,
                           void *arg);

static int
read_func(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt,
                           void *arg);
                           
static int
write_func(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt,
                           void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs_le_phy[] = {
    {
        /*** Service: LE PHY. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(LE_SERVICE_UUID16),
        .characteristics = (struct ble_gatt_chr_def[])
        {
                /*** Characteristic */
            {
                //read
                //.uuid = BLE_UUID16_DECLARE(LE_RX_CHR_UUID16),
                .uuid = BLE_UUID128_DECLARE(0xcc,0x97,0x00,0x22,0x80,0x7c,0x0b,0x85,
                                            0xab,0x42,0x7e,0xfa,0x01,0xda,0x61,0x52),
                .access_cb = read_func,
                .val_handle = &read_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
            }, 
            {
                //write
                //.uuid = BLE_UUID16_DECLARE(LE_TX_CHR_UUID16),
                .uuid = BLE_UUID128_DECLARE(0xcc,0x97,0x00,0x22,0x80,0x7c,0x0b,0x85,
                                            0xab,0x42,0x7e,0xfa,0x02,0xda,0x61,0x52),
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
                .access_cb = write_func,
                .val_handle = &write_val_handle,
            },
            {
                0, /* No more characteristics in this service. */
            }
        },
    },

    {
        0, /* No more services. */
    },
};

static int read_func(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg)
{
    const ble_uuid_t *uuid;
    int rc;
    int len;

    uuid = ctxt->chr->uuid;
    MODLOG_DFLT(INFO, "uuid:%d", uuid->type);
    rc = os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

static int write_func(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg)
{
    MODLOG_DFLT(INFO, "Data from client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

static int
gatt_svr_chr_access_le_phy(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt,
                           void *arg)
{
    const ble_uuid_t *uuid;
    char def_string[12] = "hello world";
    int rc;
    int len;
    uint16_t copied_len;
    uuid = ctxt->chr->uuid;

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */

    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(LE_SERVICE_UUID16)) == 0) {
        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
        //read
            rc = os_mbuf_append(ctxt->om, &def_string, strlen(def_string));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
        //write
            len = OS_MBUF_PKTLEN(ctxt->om);
            if (len > 0) {
                le_phy_val = (uint8_t *)malloc(len * sizeof(uint8_t));
                if (le_phy_val) {
                    rc = ble_hs_mbuf_to_flat(ctxt->om, le_phy_val, len, &copied_len);
                    if (rc == 0) {
                        MODLOG_DFLT(INFO, "Write received of len = %d", copied_len);
                        return 0;
                    } else {
                        MODLOG_DFLT(ERROR, "Failed to receive write characteristic");
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    /* Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
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
gatt_svr_init_le_phy(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs_le_phy);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs_le_phy);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
