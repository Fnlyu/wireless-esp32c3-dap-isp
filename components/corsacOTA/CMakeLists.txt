# Use as component
set(COMPONENT_ADD_INCLUDEDIRS "./src")
set(COMPONENT_SRCS "./src/corsacOTA.c"
    "./src/ymodem_transfer.c")

set(COMPONENT_REQUIRES app_update mbedtls freertos)

register_component()