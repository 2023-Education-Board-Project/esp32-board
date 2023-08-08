set(srcs "main.c"
    "gatt_server.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ".")
