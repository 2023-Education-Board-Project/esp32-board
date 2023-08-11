set(srcs "main.c"
    "gatt_server.c"
    "lvgl_ui.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "./include")
