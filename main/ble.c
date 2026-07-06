#include "ble.h"
#include "usb.h"
#include "main.h"

#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_defs.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_defs.h>

#include <esp_hidh.h>
#include <esp_hid_common.h>

static const char *TAG = "ble";

#define APPEARANCE_KEYBOARD 0x03C1   // Appearance BLE "Keyboard"
#define HID_SERVICE_UUID_16 0x1812   // Service HID
#define SCAN_SECONDS        5

typedef struct {
	esp_bd_addr_t bda;
	uint8_t       addr_type;
} open_target_t;

static QueueHandle_t open_queue = NULL;
static volatile bool connected  = false;

static bool adv_is_hid_keyboard(uint8_t *adv, uint8_t adv_len) {
	uint8_t len = 0;

	uint8_t *appearance = esp_ble_resolve_adv_data(adv, ESP_BLE_AD_TYPE_APPEARANCE, &len);
	if (appearance && len >= 2) {
		uint16_t val = appearance[0] | (appearance[1] << 8);
		if (val == APPEARANCE_KEYBOARD)
			return true;
	}

	const uint8_t types[] = { ESP_BLE_AD_TYPE_16SRV_CMPL, ESP_BLE_AD_TYPE_16SRV_PART };
	for (int t = 0; t < 2; t++) {
		uint8_t *uuid = esp_ble_resolve_adv_data(adv, types[t], &len);
		for (int i = 0; uuid && (i + 1) < len; i += 2) {
			uint16_t u = uuid[i] | (uuid[i + 1] << 8);
			if (u == HID_SERVICE_UUID_16)
				return true;
		}
	}
	return false;
}

static void start_scan(void) {
	static esp_ble_scan_params_t scan_params = {
		.scan_type          = BLE_SCAN_TYPE_ACTIVE,
		.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
		.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
		.scan_interval      = 0x50,
		.scan_window        = 0x30,
		.scan_duplicate     = BLE_SCAN_DUPLICATE_ENABLE,
	};
	esp_ble_gap_set_scan_params(&scan_params);
}

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
	switch (event) {
	case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
		ESP_LOGI(TAG, "scanning for a BLE keyboard in pairing mode...");
		esp_ble_gap_start_scanning(SCAN_SECONDS);
		break;

	case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		esp_ble_gap_cb_param_t *r = param;
		if (r->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
			if (!connected && adv_is_hid_keyboard(r->scan_rst.ble_adv,
					r->scan_rst.adv_data_len + r->scan_rst.scan_rsp_len)) {
				ESP_LOGI(TAG, "keyboard found: %02x:%02x:%02x:%02x:%02x:%02x",
					r->scan_rst.bda[0], r->scan_rst.bda[1], r->scan_rst.bda[2],
					r->scan_rst.bda[3], r->scan_rst.bda[4], r->scan_rst.bda[5]);
				open_target_t tgt = { .addr_type = r->scan_rst.ble_addr_type };
				memcpy(tgt.bda, r->scan_rst.bda, sizeof(esp_bd_addr_t));
				esp_ble_gap_stop_scanning();
				xQueueSend(open_queue, &tgt, 0);
			}
		} else if (r->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
			if (!connected)
				esp_ble_gap_start_scanning(SCAN_SECONDS);
		}
		break;
	}

	case ESP_GAP_BLE_SEC_REQ_EVT:
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;

	case ESP_GAP_BLE_AUTH_CMPL_EVT:
		if (param->ble_security.auth_cmpl.success)
			ESP_LOGI(TAG, "pairing done");
		else
			ESP_LOGW(TAG, "pairing failed (0x%x)", param->ble_security.auth_cmpl.fail_reason);
		break;

	default:
		break;
	}
}

static void hidh_cb(void *args, esp_event_base_t base, int32_t id, void *event_data) {
	esp_hidh_event_t event = (esp_hidh_event_t)id;
	esp_hidh_event_data_t *p = (esp_hidh_event_data_t *)event_data;

	switch (event) {
	case ESP_HIDH_OPEN_EVENT:
		if (p->open.status == ESP_OK) {
			connected = true;
			ESP_LOGI(TAG, "keyboard connected");
		} else {
			ESP_LOGW(TAG, "open failed, rescanning");
			start_scan();
		}
		break;

	case ESP_HIDH_INPUT_EVENT:
		if (p->input.usage == ESP_HID_USAGE_KEYBOARD && p->input.length >= 3) {
			uint8_t modifier = p->input.data[0];
			uint8_t keys[6] = { 0 };
			uint16_t n = p->input.length - 2;
			if (n > 6) n = 6;
			memcpy(keys, &p->input.data[2], n);
			usb_send_keyboard_report(modifier, keys);
		}
		break;

	case ESP_HIDH_CLOSE_EVENT:
		ESP_LOGW(TAG, "keyboard disconnected, rescanning");
		connected = false;
		start_scan();
		break;

	default:
		break;
	}
}

static void open_task(void *arg) {
	open_target_t tgt;
	while (1) {
		if (xQueueReceive(open_queue, &tgt, portMAX_DELAY) != pdTRUE)
			continue;
		if (connected)
			continue;
		ESP_LOGI(TAG, "connecting to keyboard...");
		esp_hidh_dev_open(tgt.bda, ESP_HID_TRANSPORT_BLE, tgt.addr_type);
	}
}

static void configure_security(void) {
	esp_ble_auth_req_t auth = ESP_LE_AUTH_REQ_SC_BOND;   // bonding "just works"
	esp_ble_io_cap_t   iocap = ESP_IO_CAP_NONE;
	uint8_t key_size = 16;
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t rsp_key  = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, sizeof(auth));
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
}

void ble_init(void) {
	ESP_LOGI(TAG, "ble init");
	open_queue = xQueueCreate(4, sizeof(open_target_t));

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
	ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

	ESP_ERROR_CHECK(esp_bluedroid_init());
	ESP_ERROR_CHECK(esp_bluedroid_enable());

	ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));
	configure_security();

	esp_hidh_config_t hidh_config = {
		.callback         = hidh_cb,
		.event_stack_size = 4096,
		.callback_arg     = NULL,
	};
	ESP_ERROR_CHECK(esp_hidh_init(&hidh_config));

	xTaskCreate(open_task, "hid_open", 4096, NULL, 5, NULL);

	start_scan();
	ESP_LOGI(TAG, "ble init done");
}
