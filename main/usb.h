#ifndef USB_H_
#define USB_H_

#include <stdbool.h>
#include <stdint.h>

void usb_request_keypress_send(bool from_isr);
void usb_send_keyboard_report(uint8_t modifier, const uint8_t keys[6]);
void usb_init(void);

#endif // USB_H_
