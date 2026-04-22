# USB Mass Storage on Pico SD Card 

This example turns your Pico into something like an SD card USB dongle. 
You can mount the Pico's SD card on a desktop PC by connecting the Pico's USB port to the computer's USB port.

It is derived from
[lib/tinyusb/examples/device/cdc_msc](https://github.com/hathach/tinyusb/tree/4232642899362fa5e9cf0dc59bad6f1f6d32c563/examples/device/cdc_msc).
It uses the [Block Device API](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main?tab=readme-ov-file#block-device-api).

## Requirements
* You will need to
[customize](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico#customizing-for-the-hardware-configuration)
the `hw_config.c` file to match your hardware.

## Build
Follow the instructions in [Getting started with Raspberry Pi Pico-series](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).
