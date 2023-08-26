set(srcs "main.c"
    "gatt_server.c"
    "lvgl_ui.c"
    "ble_server.c"
    "block_func.c"
    "queue.c"
    )


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "./include")
