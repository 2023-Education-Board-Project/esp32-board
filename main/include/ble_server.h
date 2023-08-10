/**
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "nimble/ble.h"
#include "modlog/modlog.h"

#ifdef __cplusplus
extern "C" {
#endif


extern uint16_t rx_handle;
extern uint16_t	tx_handle;

extern ble_uuid16_t    deviceInfo_uuid;
extern ble_uuid16_t    modelNum_uuid;
extern ble_uuid16_t    serialNum_uuid;
extern ble_uuid16_t    firmwareRevision_uuid;

extern ble_uuid16_t    service_uuid;
extern ble_uuid128_t   rxChar_uuid;
extern ble_uuid128_t   txChar_uuid;

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);

#ifdef __cplusplus
}

#endif

#endif
