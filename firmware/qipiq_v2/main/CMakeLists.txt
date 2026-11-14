idf_component_register(
    SRCS "qipiq_main.c" "qipiq_audio.c" "qipiq_ui.c" "qipiq_sensors.c" "qipiq_font_18.c" "qipiq_wifi.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_timer esp_hw_support esp_event esp_netif esp_wifi wifi_provisioning)
