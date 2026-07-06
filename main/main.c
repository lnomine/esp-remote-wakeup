#include "usb.h"
#include "ble.h"
#include "gpio.h"
#include "led.h"
#include "main.h"

#include <nvs_flash.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main(void) {
	esp_log_level_set("*", ESP_LOG_INFO);

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}

	led_init();
	gpio_init();
	usb_init();
	ble_init();

	vTaskSuspend(NULL);
}
