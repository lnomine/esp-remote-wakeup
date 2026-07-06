idf_component_register(SRCS "main.c" "usb.c" "ble.c" "gpio.c" "led.c" "led_strip_encoder.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash bt esp_hid)
