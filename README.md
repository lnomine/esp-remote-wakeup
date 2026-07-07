# ESP Wakeup Keypress

A firmware for ESP32 devices to act as a USB keyboard and wake up the computer
when a HTTP request is received.

## Usage

- Edit the following settings in `sdkconfig`:

  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_SSID
  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_PASSWORD
  - CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD
  - CONFIG_ESP_WAKEUP_KEYPRESS_LED_STRIP_GPIO_NUM

- Run `01-build.sh`
- Create the file `defport-flash` and put `/dev/ttyACM0` into it (or the port
  where your ESP can be programmed)
- Create the file `defport-monitor` and put `/dev/ttyUSB0` into it (or the port
  where your ESP serial console is)
- Put your ESP into bootloader mode by pressing and holding the GPIO0 (BOOT) button
  while pressing the reset button
- Run `02-flash.sh`
- Run `03-monitor.sh`
- Press the reset button on your board to exit bootloader mode

Now if you press the button on your board, or open the URL
`http://esp-wakeup-keypress.localdomain/wakeup?pass=wakeup` an wakeup signal is
sent to your computer. You may need to change the `pass=wakeup` in the URL
if you've modified CONFIG_ESP_WAKEUP_KEYPRESS_HTTPD_PASSWORD in the
`sdkconfig` file.

## Troubleshooting

If your computer does not wake up for the virtual keypress, then create this
shell script at `/lib/systemd/system-sleep/00-esp-wakeup-enable.sh`:

```bash
#!/bin/bash

# Action script to enable wake after suspend by keyboard or mouse

if [ "$1" = post ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi

if [ "$1" = pre ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi
```
