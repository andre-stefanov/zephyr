# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_USB_CDC_ACM cdc_acm.c)
zephyr_sources_ifdef(CONFIG_USB_MASS_STORAGE msc.c)
zephyr_sources_ifdef(CONFIG_USB_DEVICE_BLUETOOTH bluetooth.c)

add_subdirectory_ifdef(CONFIG_USB_DEVICE_AUDIO audio)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_NETWORK netusb)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_HID hid)
add_subdirectory_ifdef(CONFIG_USB_DFU_CLASS dfu)
