# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES "driver")
set(COMPONENT_PRIV_REQUIRES "esp_driver_gpio esp_driver_twai")

set(COMPONENT_SRCS "canWrapper.c" )
set(COMPONENT_ADD_INCLUDEDIRS "../vwtp" "../stalkButtons" "../sysState")

register_component()