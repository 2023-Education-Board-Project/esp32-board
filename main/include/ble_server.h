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

/**
 * gatt_server.c
*/
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);

/**
 * ble_server.c
*/
int	ble_server_init(void);
void	ble_server_start(void);

#ifdef __cplusplus
}

#endif

#endif
