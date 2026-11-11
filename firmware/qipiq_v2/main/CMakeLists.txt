idf_component_register(
    SRCS "qipiq_main.c" "qipiq_audio.c" "qipiq_ui.c" "qipiq_sensors.c" "qipiq_font_18.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_timer esp_hw_support)
