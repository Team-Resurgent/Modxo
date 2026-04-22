# no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
# v3.7.0

## C/C++ Library for SD Cards on the Pico

At the heart of this library is ChaN's [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html).
It also contains a Serial Peripheral Interface (SPI) SD Card block driver for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
derived from [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html),
and a 4-bit wide Secure Digital Input Output (SDIO) driver derived from 
[ZuluSCSI-firmware](https://github.com/ZuluSCSI/ZuluSCSI-firmware). 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

## What's new
### v3.7.0
 RISC-V compatibility
### v3.6.2
Fix `setrtc` command in `examples/command_line` CLI to start the timer.
### v3.6.1
Fix failure to release locks when an error occurs while reading blocks and CMD12_STOP_TRANSMISSION also fails.
This could happen, for example, if the SD card falls out after it has been mounted.
### v3.6.0
Add `examples/usb_mass_storage` example which connects a Pico's USB mass storage (MSC) interface to an SD card,
effectively turning it into an SD card USB dongle.
### v3.5.1
Fix PlatformIO examples for earlephilhower / arduino-pico [Add new Pico SDK AON_Timer module #2489](https://github.com/earlephilhower/arduino-pico/issues/2489).
### v3.5.0
Porting to Pico 2.
### v3.4.0
Add example of direct use of the block device API. See [Block Device API](#block-device-api) and `examples/block_device`.
### v3.3.1
* Add support for PICO_BOARD pico2.
* Fix year and month calculation in `get_fattime`, which is used for file timestamps in FatFs.
### v3.3.0
Add support for running without Chip Select (CS) (formerly Slave Select [SS]). See [Running without Chip Select (CS) (formerly Slave Select [SS])](#running-without-chip-select-cs-formerly-slave-select-ss).
### v3.2.0
* Add `spi_mode` to the hardware configuration.
For SPI attached cards, SPI Mode 3 can significantly improve performance.
See [SPI Controller Configuration](#spi-controller-configuration).
* Make timeouts configurable. See [Timeouts](#timeouts).
* Add retries in SPI driver `sd_read_blocks`.
### v3.1.0
* Add support for the RP2350
### v3.0.0
* Migrate to **Raspberry Pi Pico SDK 2.0.0**
* Simplify SPI wait for DMA transfer completion, including elimination of DMA interrupt handler.
For required migration actions, see [Appendix A: Migration actions](#appendix-a-migration-actions).
### v2.6.0
* CRC performance improvements for SPI.
* Clean up `sd_write_blocks` in `sd_card_spi.c`
### v2.5.0
* Refactor SPI sd_write_blocks.
* Drop support for SD Standard Capacity Memory Card (up to and including 2 GB). 
SDSC Card uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
### v2.4.0
* Fix bug in SPI sd_write_blocks that caused single block writes to be sent with CMD25 WRITE_MULTIPLE_BLOCK
instead of CMD24 WRITE_BLOCK.
* Added
[src/include/file_stream.h](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/src/include/file_stream.h)
and
[src/src/file_stream.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/src/src/file_stream.c)
which use the C library's
[fopencookie—open a stream with custom callbacks](https://sourceware.org/newlib/libc.html#fopencookie)
API to put a buffered Standard Input/Output (stdio) wrapper around the
[FreeRTOS-Plus-FAT Standard API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/Standard_File_System_API.html).
* Added
[examples/stdio_buffering/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/stdio_buffering)
which shows how to use the C library's Standard Input/Output (stdio) buffering to achieve significant
(up to 2X) speedups in many applications.
* See [Appendix D: Performance Tuning Tips](#appendix-d-performance-tuning-tips).
### v2.3.2
Fix initialization problem when multiple SD cards share an SPI bus. The fix puts all cards into SPI mode at driver initialization time (rather than deferring until media initialization time).
### v2.3.1
Faster way to read trailing CRC bytes
### v2.2.1
Substantial performance improvement for writing large contiguous blocks of data. This is accomplished by avoiding sending "stop transmission" for as long as possible.
The improved throughput is especially noticeable on SDIO-attached cards, since the 4 bit wide SD interface is less of a bottleneck than the 1 bit wide SPI.
### v2.1.1
Added ability to statically assign DMA channels for SPI. (See [SPI Controller Configuration](#spi-controller-configuration).)
### v2.0.1
Fix miscalculation in `get_num_sectors`.
### v2.0.0
* In the hardware configuration specification, 
the `pcName` member of `sd_card_t` has been removed. 
Rationale: FatFs provides ways to customize the volume ID strings (or "drive prefixes") used
to designate logical drives (see
[FF_STR_VOLUME_ID](http://elm-chan.org/fsw/ff/doc/config.html#str_volume_id)).
`pcName` was redundant and confusing.
* A new example has been added in `examples/unix_like`. This example illustrates the use of Unix style drive prefixes. (See [Path Names on the FatFs API](http://elm-chan.org/fsw/ff/doc/filename.html)). It also demonstrates customization of the FatFs configuration file, `ffconf.h`.
* Added a new section to this README: [Customizing the *FatFs - Generic FAT Filesystem Module*](#customizing-the-fatfs---generic-fat-filesystem-module).

For required migration actions, see [Appendix A: Migration actions](#appendix-a-migration-actions).

*Note:* v1.2.4 and v1.1.2 remain available on the
[v1.2.4 branch](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/v1.2.4)
and the
[v1.1.2 branch](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/v1.1.2), repectively.
## Features:
* Supports multiple SD cards, all in a common file system
* Supports desktop compatible SD card formats
* Supports 4-bit wide SDIO by PIO, or SPI using built in SPI controllers, or both
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Supports multiple SDIO buses
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC) for data integrity
* Compatible with Pico W
* Plus all the neat features provided by [FatFs](http://elm-chan.org/fsw/ff/00index_e.html)
## Limitation:
* This library currently does not support multiple partitions on an SD card. Neither does Windows.

## Resources Used
* SPI attached cards:
  * One or two Serial Peripheral Interface (SPI) controllers may be used.
  * For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
  * For each SD card attached to an SPI controller:
      * (Optional, if there's only one SD card) A GPIO for slave (or "chip") select (SS or "CS"). (See [Running without Chip Select (CS) (formerly Slave Select [SS])](#running-without-chip-select-cs-formerly-slave-select-ss).)
      * (Optional) A GPIO for Card Detect (CD or "DET"). (See [Notes about Card Detect](#notes-about-card-detect).)
* SDIO attached cards:
  * A PIO block
  * Two DMA channels claimed with `dma_claim_unused_channel`
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` or `irq_set_exclusive_handler` (configurable) and enabled.
  * Six GPIOs for signal pins, and, optionally, another for CD (Card Detect). Four pins must be at fixed offsets from D0 (which itself can be anywhere):
    * CLK_gpio = D0_gpio - 2.
    * D1_gpio = D0_gpio + 1;
    * D2_gpio = D0_gpio + 2;
    * D3_gpio = D0_gpio + 3;

For the complete 
[examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/command_line) application, 
configured for one SPI attached card and one SDIO-attached card, release build, 
as reported by link flag `-Wl,--print-memory-usage`:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      167056 B         2 MB      7.97%
             RAM:       17524 B       256 KB      6.68%
```
A `MinSizeRel` build for a single SPI-attached card:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      124568 B         2 MB      5.94%
             RAM:       12084 B       256 KB      4.61%
```
A `MinSizeRel` build configured for three SPI-attached and one SDIO-attached SD cards:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      133704 B         2 MB      6.38%
             RAM:       21172 B       256 KB      8.08%
```
## Performance
Writing and reading a file of 200 MiB of psuedorandom data on the same 
[Silicon Power 3D NAND U1 32GB microSD card](https://www.amazon.com/gp/product/B07RSXSYJC/) inserted into a 
[Pico Stackable, Plug & Play SD Card Expansion Module](https://forums.raspberrypi.com/viewtopic.php?t=356864)
at the default Pico system clock frequency (`clk_sys`) of 125 MHz, `Release` build, using the command
`big_file_test bf 200 `*x*,
once on SPI and one on SDIO.

* SDIO:
  * Writing
    * Elapsed seconds 19.1
    * Transfer rate 10.5 MiB/s (11.0 MB/s), or 10703 KiB/s (10960 kB/s) (87683 kb/s)
  * Reading
    * Elapsed seconds 16.3
    * Transfer rate 12.3 MiB/s (12.9 MB/s), or 12550 KiB/s (12851 kB/s) (102807 kb/s)

* SPI:
  * Writing...
    * Elapsed seconds 68.6
    * Transfer rate 2.92 MiB/s (3.06 MB/s), or 2986 KiB/s (3057 kB/s) (24458 kb/s)
  * Reading...
    * Elapsed seconds 72.7
    * Transfer rate 2.75 MiB/s (2.88 MB/s), or 2816 KiB/s (2883 kB/s) (23065 kb/s)

Results from a
[port](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/examples/command_line/tests/bench.c)
of
[SdFat's bench](https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino):

* SDIO:
  ```
  ...
  write speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  11372.8,7731,5540,5719
  9532.5,19400,5515,6835
  ...
  read speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  13173.1,5221,4940,4978
  13173.1,5220,4940,4972 
  ...
  ```
* SPI:
  ```
  ...
  write speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  3160.3,31060,20359,20686
  3204.7,21576,20338,20418
  ...
  read speed and latency
  speed,max,min,avg
  KB/Sec,usec,usec,usec
  2970.5,22491,22004,22061
  2970.5,22492,21997,22057
  ...
  ```

## Choosing the Interface Type(s)
The main reason to use SDIO is for the much greater speed that the 4-bit wide interface gets you. 
However, you pay for that in pins. 
SPI can get by with four GPIOs for the first card and one more for each additional card.
SDIO needs at least six GPIOs, and the 4 bits of the data bus have to be on consecutive GPIOs.
It is possible to put more than one card on an SDIO bus (each card has an address in the protocol), but at the higher speeds (higher than this implementation can do) the tight timing requirements don't allow it. I haven't tried it.
Running multiple SD cards on multiple SDIO buses works, but it does require a lot of pins and PIO resources.

You can mix and match the attachment types.
One strategy: use SDIO for cache and SPI for backing store. 
A similar strategy that I have used: SDIO for fast, interactive use, and SPI to offload data.

## Notes about FreeRTOS
This library is not specifically designed for use with FreeRTOS. 
For use with FreeRTOS, I suggest you consider [FreeRTOS-FAT-CLI-for-RPi-Pico](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico) instead. That implementation is designed from the ground up for FreeRTOS.

While FatFs has some support for “re-entrancy” or thread safety (where "thread" == "task", in FreeRTOS terminology), it is limited to operations such as:

- f_read
- f_write
- f_sync
- f_close
- f_lseek
- f_closedir
- f_readdir
- f_truncate

There does not appear to be sufficient FAT and directory locking in FatFs to make operations like `f_mkdir`, `f_chdir` and `f_getcwd` thread safe. If your application has a static directory tree, it should be OK in a multi-tasking application with FatFs. Otherwise, you probably need to add some additional locking. 

Then, there are the facilities used for mutual exclusion and various ways of waiting (delay, wait for interrupt, etc.). This library uses the Pico SDK facilities (which are oriented towards multi-processing on the two cores), but FreeRTOS-FAT-CLI-for-RPi-Pico uses the FreeRTOS facilities, which put the waiting task into a Blocked state, allowing other, Ready tasks to run. 

FreeRTOS-FAT-CLI-for-RPi-Pico is designed to maximize parallelism. So, if you have two cores and multiple SD card buses (SPI or SDIO), multiple FreeRTOS tasks can keep them all busy simultaneously.

## Notes about Arduino / PlatformIO
What you're probably looking for is [SdFat](https://github.com/greiman/SdFat).
[Also, see [SdFat-beta](https://github.com/greiman/SdFat-beta)].

However, this library can be used with Arduino / PlatformIO. 
See [examples/PlatformIO](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/PlatformIO).

## Hardware
### My boards
* [Pico SD Card Development Board](https://forums.raspberrypi.com/viewtopic.php?p=2123146#p2123146)
![PXL_20230726_200951753a](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/986ff919-e39e-40ef-adfb-78407f6e1e41)

* [Pico Stackable, Plug & Play SD Card Expansion Module](https://forums.raspberrypi.com/viewtopic.php?t=356864)
![PXL_20230926_212422091](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/7edfea8c-59b0-491c-8321-45487bce9693)

### Prewired boards with SD card sockets
There are a variety of RP2040 boards on the market that provide an integrated µSD socket. As far as I know, most are useable with this library.
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) works on SPI1. Looks fine for 4-bit wide SDIO.
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7). SDIO? For sure it could work with one bit SDIO, but I don't know about 4-bit. It looks like it *can* work, depending on what other functions you need on the board.
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well on SPI1. For SDIO, the data lines are consecutive, but in the reverse order! I think that it could be made to work, but you might have to do some bit twiddling. A downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
* [Challenger RP2040 SD/RTC](https://ilabs.se/challenger-rp2040-sd-rtc-datasheet/) looks usable for SPI only. 
* [RP2040-GEEK](https://www.waveshare.com/wiki/RP2040-GEEK) This looks capable of 4 bit wide SDIO.
* Here is one list of RP2040 boards: [earlephilhower/arduino-pico: Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) Only a fraction of them have an SD card socket.
  
### Rolling your own
Prerequisites:
* Raspberry Pi Pico or some other kind of RP2040 board
* Something like the [Adafruit Micro SD SPI or SDIO Card Breakout Board](https://www.adafruit.com/product/4682)[^3] or [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)

  ***Warning***: Avoid Aduino breakout boards like these: [Micro SD Storage Board Micro SD Card Modules](https://smile.amazon.com/gp/product/B07XF4Q1TT/). 
  They are designed for 5 V Arduino signals. 
  Many use simple resistor dividers to drop the signal voltage, and will not work properly with the 3.3 V Raspberry Pi Pico. 
  However, see [The 5V Arduino SD modules might work with a simple trick](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/83).
  
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~10 kΩ - 50 kΩ resistors for pull-ups
* (Optional) 100 nF, 1 µF, and 10 µF capacitors for decoupling
* (Optional) 22 µH inductor for decoupling

![image](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-pinout.svg "Pinout")
<!--
|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |
-->

Please see [here](https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing) for an example wiring table for an SPI attached card and an SDIO attached card on the same Pico. 
SPI and SDIO at 31.5 MHz are pretty demanding electrically. You need good, solid wiring, especially for grounds. A printed circuit board with a ground plane would be nice!

<!--
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1473.JPG "Prototype")
-->
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232043568.jpg "Protoboard, top")
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232026240_3.jpg "Protoboard, bottom")

### Construction
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use none, either or both of the Pico's SPIs.
* You can choose to use zero or more PIO SDIO interfaces. [However, currently, the library has only been tested with zero or one.]
I don't know that there's much call for it.
* It's possible to put more than one card on an SDIO bus, but there is currently no support in this library for it.
* For SDIO, data lines D0 - D3 must be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
Furthermore, the CMD signal must be on GPIO D0 GPIO number - 2, modulo 32. (This can be changed in the PIO code.)
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors and other electrical considerations
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector or tristateable push-pull, depending on the type of card.
[MMC](https://en.wikipedia.org/wiki/MultiMediaCard)s use an open collector bus, so it is imperative to pull this up if you want compatibility with MMCs.
However, modern SD cards use strong push-pull tristateable outputs and shouldn't need this pull up.
On some SD cards, you can configure the card's output drivers using the Driver Stage Register (DSR).[^4]).
The Pico internal `gpio_pull_up` is weak: around 56uA or 60kΩ.
If a pull up is needed, it's best to add an external pull up resistor of around 5-50 kΩ to 3.3v.
The internal `gpio_pull_up` can be disabled in the hardware configuration by setting the `no_miso_gpio_pull_up` attribute of the `spi_t` object.
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config), don't let the CS float on the unused one. At power up the CS/DAT3 line has a 50 kΩ pull up enabled in the SD card, but I wouldn't necessarily count on that. It will be disabled if the card is initialized,
and it won't be enabled again until the card is power cycled. Also, the RP2040 defaults GPIO pins to pull down, which might override the SD card's pull up.
* Driving the SD card directly with the GPIOs is not ideal. Take a look at the [CM1624](https://www.onsemi.com/pdf/datasheet/cm1624-d.pdf). Unfortunately, it's a tiny little surface mount part -- not so easy to work with, but the schematic in the data sheet is still instructive. Besides the pull up resistors, it's a good idea to have 25 - 100 Ω series source termination resistors in each of the signal lines. 
This gives a cleaner signal, allowing higher baud rates. Even if you don't care about speed, it also helps to control the slew rate and current, which can reduce EMI and noise in general. (This can be important in audio applications, for example.) Ideally, the resistor should be as close as possible to the driving end of the line. That would be the Pico end for CS, SCK, MOSI, and the SD card end for MISO. For SDIO, the data lines are bidirectional, so, ideally, you'd have a source termination resistor at each end. Practically speaking, the clock is by far the most important to terminate, because each edge is significant. The other lines probably have time to bounce around before being clocked. Ideally, the resistance should be towards the low end for fat PCB traces, and towards the high end for flying wires, but if you have a drawer full of 47 Ω resistors they'll probably work well enough.
* It can be helpful to add a decoupling capacitor or three (e.g., 100 nF, 1 µF, and 10 µF) between 3.3 V and GND on the SD card. ChaN also [recommends](http://elm-chan.org/docs/mmc/mmc_e.html#hotplug) putting a 22 µH inductor in series with the Vcc (or "Vdd") line to the SD card.
* Good grounds are very important.
Remember, the current for all of the signal lines will flow back through the grounds.
There is a reason that the Pico devotes eight pins to GND.
* If your system allows hot removal and insertion of an SD card,
remember to allow for floating lines when the card is removed
and inrush current when the card is inserted. See [Cosideration to Bus Floating and Hot Insertion](http://elm-chan.org/docs/mmc/mmc_e.html#hotplug).
* Note: the [Adafruit Breakout Board](https://learn.adafruit.com/assets/93596) takes care of the pull ups and decoupling caps, but the Sparkfun one doesn't. And, you can never have too many decoupling caps.

## Notes about Card Detect
* There is one case in which Card Detect can be important: when the user can hot swap the physical card while the file system is mounted. In this case, the file system might have no way of knowing that the card was swapped, and so it will continue to assume that its prior knowledge of the FATs and directories is still valid. File system corruption and data loss are the likely results.
* If Card Detect is used, in order to detect a card swap there needs to be a way for the application to be made aware of a change in state when the card is removed. This could take the form of a GPIO interrupt (see
[examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/6c523f713ffa80dfeed2a61444a9ac58ac2bd1f8/examples/command_line/main.cpp#L652)), 
or polling.
* Some workarounds for absence of Card Detect:
  * Periodically poll sd_test_com() which can be called any time after sd_init_driver() is called. This function minimally accesses the bus to check for the presence of an SD card. The internals of this call automatically flags the SD interface for reinitialization when false is returned. If false is returned when the previous call returned true, it is important to invalidate any file handles that are still opened, unmount, and reset any mounted flags. Then don't try to remount until sd_test_com() returns true once again.
  * If you don't care much about performance or battery life, you could mount the card before each access and unmount it after. This might be a good strategy for a slow data logging application, for example.
  * Some other form of polling: if the card is periodically accessed at rate faster than the user can swap cards, then the temporary absence of a card will be noticed, so a swap will be detected. For example, if a data logging application writes a log record to the card once per second, it is unlikely that the user could swap cards between accesses.

## Running without Chip Select (CS) (formerly Slave Select [SS])
If you have only one SD card, and you are short on GPIOs, you may be able to run without CS (SS).
The idea is that if you have only one SD card, why not leave it permanently selected?
In this minimal configuration, only three GPIOs are required: CLK (SCK), DI (MOSI), and DO (MISO).

I know of no guarantee that this will work for all SD cards.
The [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) says
> Every command or data block is
> built of 8-bit bytes and is byte aligned with the CS signal...
> The card starts to count SPI bus clock cycle at the assertion of the CS signal...
> The host
> starts every bus transaction by asserting the CS signal low.

It doesn't say what happens if the CS signal is always asserted.
However, it worked for me with:
* [Silicon Power 3D NAND U1 32GB microSD card](https://www.amazon.com/gp/product/B07RSXSYJC/)
* [SanDisk 16GB Ultra microSDHC UHS-I Memory Card ](https://www.amazon.com/gp/product/B089DPCJS1/ref=ppx_yo_dt_b_search_asin_title?th=1)
* [PNY 16GB Elite Class 10 U1 microSDHC Flash Memory Card](https://www.amazon.com/gp/product/B08QDN7CVN/ref=ppx_yo_dt_b_search_asin_title)

You will need to pull down the CS line on the SD card with hardware. (I.e., connect CS to GND. CS is active low.)

In the hardware configuration definition, set `ss_gpio` to -1.
See [An instance of `sd_spi_if_t` describes the configuration of one SPI to SD card interface.](#an-instance-of-sd_spi_if_t-describes-the-configuration-of-one-spi-to-sd-card-interface).

## Firmware
### Procedure
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  ```bash
  git clone --recurse-submodules https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
  ```
* Customize:
  * Configure the code to match the hardware: see section 
  [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration), below.
  * Customize [ffconf.h](http://elm-chan.org/fsw/ff/doc/config.html) as desired
  * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer. 
(See *4.1. Serial input and output on Raspberry Pi Pico* in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) and *2.7.1. Standard Input/Output (stdio) Support* in [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf).) 
* Build:
```bash
   cd no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/examples/command_line
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  * See [examples/command_line/README.md](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/command_line/README.md) for operation.
<!--
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/IMG_1481.JPG "Prototype")
-->

## Customizing for the Hardware Configuration 
This library can support many different hardware configurations. 
Therefore, the hardware configuration is not defined in the library[^1]. 
Instead, the application must provide it. 
The configuration is defined in "objects" of type `spi_t` (see `sd_driver/spi.h`), 
`sd_spi_if_t`, `sd_sdio_if_t`, and `sd_card_t` (see `sd_driver/sd_card.h`). 
* Instances of `sd_card_t` describe the configuration of SD card sockets.
* Each instance of `sd_card_t` is associated (one to one) with an `sd_spi_if_t` or `sd_sdio_if_t` interface object, 
and points to it with `spi_if_p` or `sdio_if_p`[^5].
* Instances of `sdio_if_p` specify the configuration of an SDIO/PIO interface.
* Each instance of `sd_spi_if_t` is assocated (many to one) with an instance of `spi_t` and points to it with `spi_t *spi`. (It is a many to one relationship because multiple SD cards can share a single SPI bus, as long as each has a unique slave (or "chip") select (SS, or "CS") line.) It describes the configuration of a specific SD card's interface to a specific SPI hardware component.
* Instances of `spi_t` describe the configuration of the RP2040 SPI hardware components used.
There can be multiple objects (or "instances") of all three types.
Attributes (or "fields", or "members") of these objects specify which pins to use for what, baud rates, features like Card Detect, etc.
* Generally, anything not specified will default to `0` or `false`. (This is the user's responsibility if using Dynamic Configuration, but in a Static Configuration 
[see [Static vs. Dynamic Configuration](#static-vs-dynamic-configuration)], 
the C runtime initializes static memory to 0.)

![Illustration of the configuration dev_brd.hw_config.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/0eedadea-f6cf-44cb-9b76-544ec74287d2)

Illustration of the configuration 
[dev_brd.hw_config.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/examples/command_line/dev_brd.hw_config.c)

### An instance of `sd_card_t` describes the configuration of one SD card socket
  ```C
struct sd_card_t {
    sd_if_t type;
    union {
        sd_spi_if_t *spi_if_p;
        sd_sdio_if_t *sdio_if_p;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect
    bool card_detect_use_pull;
    bool card_detect_pull_hi;
//...
}
```
* `type` Type of interface: either `SD_IF_SPI` or `SD_IF_SDIO`
* `spi_if_p` or `sdio_if_p` Pointer to the instance `sd_spi_if_t` or `sd_sdio_if_t` that drives this SD card
* `use_card_detect` Whether or not to use Card Detect, meaning the hardware switch featured on some SD card sockets. This requires a GPIO pin.
* `card_detect_gpio` Ignored if not `use_card_detect`. GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` Ignored if not `use_card_detect`. What the GPIO read returns when a card is present (Some sockets use active high, some low)
* `card_detect_use_pull` Ignored if not `use_card_detect`. If true, use the `card_detect_gpio`'s pad's Pull Up / Pull Down resistors; 
if false, no pull resistor is applied. 
Often, a Card Detect Switch is just a switch to GND or Vdd, 
and you need a resistor to pull it one way or the other to make logic levels.
* `card_detect_pull_hi` Ignored if not `use_card_detect`. Ignored if not `card_detect_use_pull`. Otherwise, if true, pull up; if false, pull down.

### An instance of `sd_sdio_if_t` describes the configuration of one SDIO to SD card interface.
  ```C
typedef struct sd_sdio_if_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    uint baud_rate;
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength CLK_gpio_drive_strength;
    enum gpio_drive_strength CMD_gpio_drive_strength;
    enum gpio_drive_strength D0_gpio_drive_strength;
    enum gpio_drive_strength D1_gpio_drive_strength;
    enum gpio_drive_strength D2_gpio_drive_strength;
    enum gpio_drive_strength D3_gpio_drive_strength;
//...
} sd_sdio_t;
```
Specify `D0_gpio`, but pins `CLK_gpio`, `D1_gpio`, `D2_gpio`, and `D3_gpio` are at offsets from pin `D0_gpio` and are set implicitly.
The offsets are determined by `sd_driver\SDIO\rp2040_sdio.pio`.
  As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30,
    which is -2 in mod32 arithmetic, so:
    
  * CLK_gpio = D0_gpio - 2
  * D1_gpio = D0_gpio + 1
  * D2_gpio = D0_gpio + 2
  * D3_gpio = D0_gpio + 3 

These pin assignments are set implicitly and must not be set explicitly.
* `CLK_gpio` RP2040 GPIO to use for Clock (CLK).
Implicitly set to `(D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32` where `SDIO_CLK_PIN_D0_OFFSET` is defined in `sd_driver/SDIO/rp2040_sdio.pio`.
As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30, which is -2 in mod32 arithmetic, so:
  * CLK_gpio = D0_gpio - 2
* `CMD_gpio` RP2040 GPIO to use for Command/Response (CMD)
* `D0_gpio` RP2040 GPIO to use for Data Line [Bit 0]. The PIO code requires D0 - D3 to be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
* `D1_gpio` RP2040 GPIO to use for Data Line [Bit 1]. Implicitly set to D0_gpio + 1.
* `D2_gpio` RP2040 GPIO to use for Data Line [Bit 2]. Implicitly set to D0_gpio + 2.
* `D3_gpio` RP2040 GPIO to use for Card Detect/Data Line [Bit 3]. Implicitly set to D0_gpio + 3.
* `SDIO_PIO` Which PIO block to use. Defaults to `pio0`. Can be changed to avoid conflicts. 
If you try to use multiple SDIO-attached SD cards simultaneously on the same PIO block,
contention might lead to timeouts.
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.
* `use_exclusive_DMA_IRQ_handler` If true, the IRQ handler is added with the SDK's `irq_set_exclusive_handler`. The default is to add the handler with `irq_add_shared_handler`, so it's not exclusive. 
* `baud_rate` The frequency of the SDIO clock in Hertz.
  This may be no higher than the system clock frequency divided by `CLKDIV` in `sd_driver\SDIO\rp2040_sdio.pio`, which is currently four.
  For example, if the system clock frequency is 125 MHz,
  `baud_rate` cannot exceed 31250000 (31.25 MHz). The default is `clk_sys` / 12. 
  
  The `baud_rate` is derived from the system core clock (`clk_sys`).
  `sm_config_set_clkdiv` sets the state machine clock divider
  in a PIO state machine configuration
  from a floating point value we'll call "clk_div".
  This is used to divide the system clock frequency (`clk_sys`) to get a ratio to pass to the SDK's
  [sm_config_set_clkdiv](https://www.raspberrypi.com/documentation//pico-sdk/hardware.html#group_sm_config_1ga365abc6d25301810ca5ee11e5b36c763).
  The state machine clock divider is a fractional divider,
  and the jitter introduced by a fractional divisor may be unacceptable.
  "An integer clock divisor of n will cause the state machine to run 1 cycle in every n. 
  Note that for small n, the jitter introduced by a fractional divider (e.g. 2.5) may be unacceptable although it will depend on the use case."
  See the datasheet for details.
  The fractional divider essentially causes the frequency to vary in a range,
  with the average being the requested frequency.
  If the hardware is capable of running at the high end of the range,
  you might as well run at that frequency all the time.
  The higher the baud rate, the faster the data transfer.
  However, the hardware might limit the usable baud rate.
  See [Pull Up Resistors and other electrical considerations](#pull-up-resistors-and-other-electrical-considerations).
  
  The PIO state machine itself divides by `CLKDIV`,
  defined in `sd_driver\SDIO\rp2040_sdio.pio`, currently 4.
  
        baud_rate = clk_sys / (CLKDIV * clk_div)
  
  Preferrably, choose `baud_rate` for an integer clk_div.
  
  Baud rates for different `clk_sys`s and `clk_div`s:
    |         | clk_sys      | clk_sys      | clk_sys      |
    | ------- | ------------ | ------------ | ------------ |
    | clk_div | 125000000    | 133000000    | 150000000  |
    | 1.00    | 31,250,000   | 33,250,000   | 37,500,000 |
    | 2.00    | 15,625,000   | 16,625,000   | 18,750,000 |
    | 3.00    | 10,416,667   | 11,083,333   | 12,500,000 |
* `set_drive_strength` If true, enable explicit specification of output drive strengths on `CLK_gpio`, `CMD_gpio`, and `D0_gpio` - `D3_gpio`. 
The GPIOs on RP2040 have four different output drive strengths, which are nominally 2, 4, 8 and 12mA modes.
If `set_drive_strength` is false, all will be implicitly set to 4 mA.
If `set_drive_strength` is true, each GPIO's drive strength can be set individually. Note that if it is not explicitly set, it will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).

* ```
  CLK_gpio_drive_strength
  CMD_gpio_drive_strength 
  D0_gpio_drive_strength 
  D1_gpio_drive_strength 
  D2_gpio_drive_strength 
  D3_gpio_drive_strength 
  ``` 
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot. 
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.

### An instance of `sd_spi_if_t` describes the configuration of one SPI to SD card interface.
```C
typedef struct sd_spi_if_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_if_t;
```
* `spi` Points to the instance of `spi_t` that is to be used as the SPI to drive this interface
* `ss_gpio` Slave Select (SS) (or "Chip Select [CS]") GPIO for the SD card socket associated with this interface.
Set this to -1 to disable it.
(See [Running without Chip Select (CS) (formerly Slave Select [SS])](#running-without-chip-select-cs-formerly-slave-select-ss).)
*Note:* 0 is a valid GPIO number, so you must explicitly set it to -1 to disable it.
* `set_drive_strength` Enable explicit specification of output drive strength of `ss_gpio_drive_strength`. 
If false, the GPIO's drive strength will be implicitly set to 4 mA.
* `ss_gpio_drive_strength` Drive strength for the SS (or CS).
  Ignored if `set_drive_strength` is false. Otherwise, it can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
### SPI Controller Configuration
An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```C
typedef struct spi_t {
    spi_inst_t *hw_inst;  // SPI HW
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;

    /* The different modes of the Motorola SPI protocol are:
    - Mode 0: When CPOL and CPHA are both 0, data sampled at the leading rising edge of the
    clock pulse and shifted out on the falling edge. This is the most common mode for SPI bus
    communication.
    - Mode 1: When CPOL is 0 and CPHA is 1, data sampled at the trailing falling edge and
    shifted out on the rising edge.
    - Mode 2: When CPOL is 1 and CPHA is 0, data sampled at the leading falling edge
    and shifted out on the rising edge.
    - Mode 3: When CPOL is 1 and CPHA is 1, data sampled at the trailing rising edge and
    shifted out on the falling edge. */
    uint spi_mode;
    bool no_miso_gpio_pull_up;

    /* Drive strength levels for GPIO outputs:
        GPIO_DRIVE_STRENGTH_2MA, 
        GPIO_DRIVE_STRENGTH_4MA, 
        GPIO_DRIVE_STRENGTH_8MA,
        GPIO_DRIVE_STRENGTH_12MA */
    bool set_drive_strength;
    enum gpio_drive_strength mosi_gpio_drive_strength;
    enum gpio_drive_strength sck_gpio_drive_strength;

    bool use_static_dma_channels;
    uint tx_dma;
    uint rx_dma;

    // State variables:
// ...
} spi_t;
```
* `hw_inst` Identifier for the hardware SPI instance (for use in SPI functions). e.g. `spi0`, `spi1`, declared in `pico-sdk\src\rp2_common\hardware_spi\include\hardware\spi.h`
* `miso_gpio` SPI Master In, Slave Out (MISO) (also called "CIPO" or "Peripheral's SDO") GPIO number. This is connected to the SD card's Data Out (DO).
* `mosi_gpio` SPI Master Out, Slave In (MOSI) (also called "COPI", or "Peripheral's SDI") GPIO number. This is connected to the SD card's Data In (DI).
* `sck_gpio` SPI Serial Clock GPIO number. This is connected to the SD card's Serial Clock (SCK).
* `baud_rate` Frequency of the SPI Serial Clock, in Hertz. The default is `clk_sys` / 12.
  This is ultimately passed to the SDK's [spi_set_baudrate](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga37f4c04ce4165ac8c129226336a0b66c). This applies a hardware prescale and a post-divide to the *Peripheral clock* (`clk_peri`) (see section **4.4.2.3.** *Clock prescaler* in [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)). 
  The *Peripheral clock* typically,
  but not necessarily, runs from `clk_sys`.
  Practically, the hardware limits the choices for the SPI frequency to `clk_peri` divided by an even number.
  For example, if `clk_peri` is `clk_sys` and `clk_sys` is running at the default 125 MHz,
  ```C
      .baud_rate = 125 * 1000 * 1000 / 10,  // 12500000 Hz
  ```
  or
  ```C
      .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
  ```
  If you ask for 14,000,000 Hz, you'll actually get 12,500,000 Hz.
  The actual baud rate will be printed out if `USE_DBG_PRINTF` (see [Messages from the SD card driver](#messages-from-the-sd-card-driver)) is defined at compile time.
  The higher the baud rate, the faster the data transfer.
  At the maximum `clk_peri` frequency on RP2040 of 133MHz, the maximum peak bit rate in master mode is 62.5Mbps.
  However, the hardware (including the SD card) might limit the usable baud rate.
  See [Pull Up Resistors and other electrical considerations](#pull-up-resistors-and-other-electrical-considerations).
* `spi_mode` 0, 1, 2, or 3. 0 is the most common mode for SPI bus slave communication.
This controls the Motorola SPI frame format CPOL, clock polarity; and CPHA, clock phase.
SPI mode 0 (CPOL=0, CPHA=0) is the proper setting to control MMC/SDC, but mode 3 (CPOL=1, CPHA=1) also works as well in most cases[^6].
Mode 3 can be around 15% faster than mode 0, probably due to quirks of the ARM PrimeCell Synchronous Serial Port in the RP2040.
* `no_miso_gpio_pull_up` According to the standard, an SD card's DO MUST be pulled up (at least for the old MMC cards). 
However, it might be done externally. If `no_miso_gpio_pull_up` is false, the library will set the RP2040 GPIO internal pull up.
* `set_drive_strength` Specifies whether or not to set the RP2040 GPIO pin drive strength.
If `set_drive_strength` is false, all will left at the reset default of 4 mA.
If `set_drive_strength` is true, each GPIO's drive strength can be set individually.
Note that any not explicitly set will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).
* `mosi_gpio_drive_strength` SPI Master Out, Slave In (MOSI) drive strength, 
* and `sck_gpio_drive_strength` SPI Serial Clock (SCK) drive strength:
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot.
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.
* `use_static_dma_channels` If true, the DMA channels provided in `tx_dma` and `rx_dma` will be claimed with `dma_channel_claim` and used.
If false, two DMA channels will be claimed with `dma_claim_unused_channel`.
* `tx_dma` The DMA channel to use for SPI TX. Ignored if `dma_claim_unused_channel` is false
* `rx_dma` The DMA channel to use for SPI RX. Ignored if `dma_claim_unused_channel` is false
### You must provide a definition for the functions declared in `sd_driver/hw_config.h`
* `size_t sd_get_num()` Returns the number of SD cards  
* `sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given
[physical drive number](http://elm-chan.org/fsw/ff/doc/filename.html#vol).
In a static, `hw_config.c` kind of configuration, that could be the (zero origin indexed) position in the `sd_cards[]` array.
It may return NULL if the drive is not available; e.g., if it is disabled. `num` must be less than `sd_get_num()`.

### Static vs. Dynamic Configuration
The definition of the hardware configuration can either be built in at build time, which I'm calling "static configuration", or supplied at run time, which I call "dynamic configuration". 
In either case, the application simply provides an implementation of the functions declared in `sd_driver/hw_config.h`. 
* See `simple_example.dir/hw_config.c` or `example/hw_config.c` for examples of static configuration.
* See `dynamic_config_example/hw_config.cpp` for an example of dynamic configuration.
* One advantage of static configuration is that the fantastic GNU Linker (ld) strips out anything that you don't use.

### Customizing the *FatFs - Generic FAT Filesystem Module*
There are many options to configure the features of FatFs for various requirements of each project. 
The configuration options are defined in `ffconf.h`. 
See [Configuration Options](http://elm-chan.org/fsw/ff/doc/config.html).
In order to allow applications to customize `ffconf.h` without modifying this library, 
I have renamed `src\ff15\source\ffconf.h` in the FatFs distribution files included in this library.
(This is necessary because GCC always looks in the directory of the current file as the first search directory for #include "file".)
There is a somewhat customized version of `ffconf.h` in `include\ffconf.h` that is normally picked up by the library's `src\CMakeLists.txt`.
An application may provide its own tailored copy of `ffconf.h` by putting it in the include path for the library compilation.
For example, for CMake, if the customized `ffconf.h` file is in subdirectory `include/` (relative to the application's `CMakeLists.txt` file):
```CMake
target_include_directories(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico BEFORE INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
```
For an example, see `examples/unix_like`.

### Timeouts
Indefinite timeouts are normally bad practice, because they make it difficult to recover from an error.
Therefore, we have timeouts all over the place.
To make these configurable, they are collected in `sd_timeouts_t sd_timeouts` in `sd_timeouts.c`.
The definition has the `weak` attribute, so it can be overridden by user code.
For example, in `hw_config.c` you could have:
```C
sd_timeouts_t sd_timeouts = {
    .sd_command = 2000, // Timeout in ms for response
    .sd_command_retries = 3, // Times SPI cmd is retried when there is no response
//...
    .sd_sdio_begin = 1000, // Timeout in ms for response
    .sd_sdio_stopTransmission = 200, // Timeout in ms for response
};
```

## Using the Application Programming Interface
After `stdio_init_all()`, `time_init()`, and whatever other Pico SDK initialization is required, 
you may call `sd_init_driver()` to initialize the block device driver. `sd_init_driver()` is now[^2] called implicitly by `disk_initialize`, 
which is called by FatFs's [f_mount](http://elm-chan.org/fsw/ff/doc/mount.html),
but you might want to call it sooner so that the GPIOs get configured, e.g., if you want to set up a Card Detect interrupt.
You need to call `sd_init_driver()` before you can use `sd_get_drive_prefix(sd_card_t *sd_card_p)`,
so you might need to add an explicit call to `sd_init_driver` if you want to use `sd_get_drive_prefix`
to get the drive prefix to use for the `path` parameter in a call to `f_mount`.
There is no harm in calling `sd_init_driver()` repeatedly.
* Now, you can start using the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html). Typically,
  * f_mount - Register/Unregister the work area of the volume
  * f_open - Open/Create a file
  * f_write - Write data to the file
  * f_read - Read data from the file
  * f_close - Close an open file
  * f_unmount
* There is a simple example in the [examples/simple](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/simple) subdirectory.
* There is also POSIX-like API wrapper layer in `ff_stdio.h` and `ff_stdio.c`, written for compatibility with [FreeRTOS+FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html) (mainly so that I could reuse some tests from that environment.)

### Messages
Sometimes problems arise when attempting to use SD cards. At the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html) level, it can be difficult to diagnose problems. You get a [return code](http://elm-chan.org/fsw/ff/doc/rc.html), but it might just tell you `FR_NOT_READY` ("The physical drive cannot work"), 
for example, without telling you what you need to know in order to fix the problem.
The library generates messages that might help. 
These are classed into Error, Informational, and Debug messages. 

Two compile definitions control how these are handled:
* `USE_PRINTF` If this is defined and not zero, 
these message output functions will use the Pico SDK's Standard Output (`stdout`).
* `USE_DBG_PRINTF` If this is not defined or is zero or `NDEBUG` is defined, 
`DBG_PRINTF` statements will be effectively stripped from the code.

Messages are sent using `EMSG_PRINTF`, `IMSG_PRINTF`, and `DBG_PRINTF` macros, which can be redefined (see 
[my_debug.h](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/src/include/my_debug.h)). 
By default, these call `error_message_printf`, `info_message_printf`, and `debug_message_printf`, 
which are implemented as
[weak](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html) functions, 
meaning that they can be overridden by strongly implementing them in user code. 
If `USE_PRINTF` is defined and not zero, the weak implementations will write to the Pico SDK's stdout. Otherwise, they will format the messages into strings and forward to `put_out_error_message`, `put_out_info_message`, and `put_out_debug_message`. These are implemented as weak functions that do nothing. You can override these to send the output somewhere.

## C++ Wrapper
At heart, this is a C library, but I have made a (thin) C++ wrapper for it: `include\FatFsSd.h`.
For details on most of the functions, refer to [FatFs - Generic FAT Filesystem Module, "Application Interface"](http://elm-chan.org/fsw/ff/00index_e.html).

See `examples\PlatformIO\one_SPI.C++\src\main.cpp` for an example of the use of this API.

### namespace FatFsNs
The C++ API is in namespace `FatFsNs`, to avoid name clashes with other packages (e.g.: SdFat).

### class FatFs
This is a pure static class that represents the global file system as a whole. 
It stores the hardware configuration objects internally, 
which is useful in Arduino-like environments where you have a `setup` and a `loop`.
The objects can be constructed in the `setup` and added to `FatFs` and the copies won't go out of scope
when control leaves `setup` and goes to `loop`. 
It automatically provides these functions required internally by the library:

* `size_t sd_get_num()` Returns the number of SD cards  
* `sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

Static Public Member Functions:
* `static SdCard* add_sd_card(SdCard& SdCard)` Use this to add an instance of `SdCard`or `sd_card_t` to the configuration. Returns a pointer that can be used to access the newly created SDCard object.
* `static size_t        SdCard_get_num ()` Get the number of SD cards in the configutation
* `static SdCard *SdCard_get_by_num (size_t num)` Get pointer to an `SdCard` by (zero-origin) index
* `static SdCard *SdCard_get_by_name (const char *const name)` Get pointer to an `SdCard` by logical drive identifier
* `static FRESULT       chdrive (const TCHAR *path)` Change current drive to given logical drive
* `static FRESULT       setcp (WORD cp)` Set current code page
* `static bool  begin ()` Initialize driver and all SD cards in configuration

### class SdCard
Represents an SD card socket. It is generalized: the SD card can be either SPI or SDIO attached.

Public Member Functions:
* `const char * get_name ()` Get the the FatFs [logical drive](http://elm-chan.org/fsw/ff/doc/filename.html#vol) identifier.
* `FRESULT      mount ()` Mount SD card
* `FRESULT      unmount ()` Unmount SD card
* `FRESULT      format ()` Create a FAT volume with defaults
* `FATFS *      fatfs ()` Get filesystem object structure (FATFS)
* `uint64_t     get_num_sectors ()` Get number of blocks on the drive
* `void cidDmp(printer_t printer)` Print information from Card IDendtification register
* `void csdDmp(printer_t printer)` Print information from Card-Specific Data register

Static Public Member Functions
* `static FRESULT mkfs (const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len) Create a FAT volume
* `static FRESULT fdisk (BYTE pdrv, const LBA_t ptbl[], void* work)` Divide a physical drive into some partitions
* `static FRESULT getfree (const TCHAR *path, DWORD *nclst, FATFS **fatfs)` Get number of free blocks on the drive
* `static FRESULT getlabel (const TCHAR *path, TCHAR *label, DWORD *vsn)` Get volume label
* `static FRESULT setlabel (const TCHAR *label)` Set volume label

### class File

 * `FRESULT      open (const TCHAR *path, BYTE mode)` Open or create a file 
 * `FRESULT      close ()` Close an open file object
 * `FRESULT      read (void *buff, UINT btr, UINT *br)` Read data from the file
 * `FRESULT      write (const void *buff, UINT btw, UINT *bw)` Write data to the file 
 * `FRESULT      lseek (FSIZE_t ofs)` Move file pointer of the file object
 * `FRESULT      expand (uint64_t file_size)` Prepare or allocate a contiguous data area to the file with default option
 * `FRESULT      truncate ()` Truncate the file 
 * `FRESULT      sync ()` Flush cached data of the writing file
 * `int  putc (TCHAR c)` Put a character to the file 
 * `int  puts (const TCHAR *str)` Put a string to the file
 * `int  printf (const TCHAR *str,...)` Put a formatted string to the file
 * `TCHAR *      gets (TCHAR *buff, int len)` Get a string from the file
 * `bool         eof ()` Test for end-of-file
 * `BYTE         error ()` Test for an error
 * `FSIZE_t      tell ()` Get current read/write pointer
 * `FSIZE_t      size ()` Get size in bytes
 * `FRESULT      rewind ()` Move the file read/write pointer to 0 (beginning of file)
 * `FRESULT      forward (UINT(*func)(const BYTE *, UINT), UINT btf, UINT *bf)` Forward data to the stream
 * `FRESULT      expand (FSIZE_t fsz, BYTE opt)` Prepare or allocate a contiguous data area to the file

### class Dir 

Public Member Functions:
* `FRESULT      rewinddir ()` Rewind the read index of the directory object 
* `FRESULT      rmdir (const TCHAR *path)` Remove a sub-directory
* `FRESULT      opendir (const TCHAR *path)` Open a directory
* `FRESULT      closedir ()` Close an open directory
* `FRESULT      readdir (FILINFO *fno)` Read a directory item
* `FRESULT      findfirst (FILINFO *fno, const TCHAR *path, const TCHAR *pattern)` Open a directory and read the first item matched
* `FRESULT      findnext (FILINFO *fno)` Read a next item matched
Static Public Member Functions:
* `static FRESULT       mkdir (const TCHAR *path)` Create a sub-directory
* `static FRESULT       unlink (const TCHAR *path)` Remove a file or sub-directory
* `static FRESULT       rename (const TCHAR *path_old, const TCHAR *path_new)` Rename/Move a file or sub-directory
* `static FRESULT       stat (const TCHAR *path, FILINFO *fno)` Check existance of a file or sub-directory
* `static FRESULT       chmod (const TCHAR *path, BYTE attr, BYTE mask)` Change attribute of a file or sub-directory
* `static FRESULT       utime (const TCHAR *path, const FILINFO *fno)` Change timestamp of a file or sub-directory
* `static FRESULT       chdir (const TCHAR *path)` Change current directory
* `static FRESULT       chdrive (const TCHAR *path)` Change current drive
* `static FRESULT       getcwd (TCHAR *buff, UINT len)` Retrieve the current directory and drive

## PlatformIO Libary
This library is available at https://registry.platformio.org/libraries/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.
It is currently running with 
```
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board_build.core = earlephilhower
```

## Block Device API
If you don't require a filesystem on an SD card,
or if you want to use a different filesystem,
you can operate at the block device level.
At the block device interface, the SD card appears to contain a long sequence of
numbered blocks of 512 bytes each. I.e., the smallest addressable unit is a block of 512 bytes.
The address of a block is its "logical block address" (LBA).
Blocks can be addressed by their LBA and read and written individually or as sequences with a starting address and length.

This API implements the *Media Access Interface* described in [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html) (also see *Required Functions* in [FatFs Module Application Note](http://elm-chan.org/fsw/ff/doc/appnote.html)).
The declarations are in `src/ff15/source/diskio.h`.

For an example of the use of this API, see `examples/block_device`.

## Next Steps
* There is a example data logging application in `data_log_demo.c`. 
It can be launched from the `examples/command_line` CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

* If you want to use no-OS-FatFS-SD-SDIO-SPI-RPi-Pico as a library embedded in another project, use something like:
  ```bash
  git submodule add git@github.com:carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
  ```
  or
  ```bash
  git submodule add https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
  ```
  
You might also need to pick up the library in CMakeLists.txt:
```CMake
add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build)
target_link_libaries(${PROGRAM_NAME} no-OS-FatFS-SD-SDIO-SPI-RPi-Pico) 
```
(where `${PROGRAM_NAME}` is your target `project`)
and `#include "ff.h"`.

Happy hacking!

## Future Directions
You are welcome to contribute to this project! Just submit a Pull Request in GitHub. Here are some ideas for future enhancements:
* Battery saving: at least stop the SDIO clock when it is not needed
* Support 1-bit SDIO
* Try multiple cards on a single SDIO bus
* [RP2040: Enable up to 42 MHz SDIO bus speed](https://github.com/ZuluSCSI/ZuluSCSI-firmware/tree/rp2040_highspeed_sdio)
* SD UHS Double Data Rate (DDR): clock data on both edges of the clock
<!--
![image](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/8a28782e-84c4-40c8-8757-a063a4b83292)
-->
## Appendix A: Migration actions
### Migrating from v2
* **Raspberry Pi Pico SDK** minimum version is now 2.0.0.
* All references to the `DMA_IRQ_num` member must be removed from each instance of `spi_t` in the hardware configuration.
* All references to the `use_exclusive_DMA_IRQ_handler` member must be removed from each instance of `spi_t` in the hardware configuration.

### Migrating from v1
Any references to the `pcName` member must be removed from each instance of `sd_card_t` in the hardware configuration.

For example, if you were using a `hw_config.c` containing 
```C
static sd_card_t sd_cards[] = {  // One for each SD card
    {   // sd_cards[0]: Socket sd0
        .pcName = "0:",
        .type = SD_IF_SPI,
        // ...
```
change it to
```C
static sd_card_t sd_cards[] = {  // One for each SD card
    {   // sd_cards[0]: Socket sd0
        .type = SD_IF_SPI,
        // ...
```
See [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration).

If you had any references to `pcName` in your code, 
you can replace them with calls to `char const *sd_get_drive_prefix(sd_card_t *sd_card_p);` (`sd_card.h`).
Note: `sd_init_driver()` must be called before `sd_get_drive_prefix`.

### Migrating from no-OS-FatFS-SD-SPI-RPi-Pico [master](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/tree/master) branch
The object model for hardware configuration has changed.
If you are migrating a project from [no-OS-FatFS-SD-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico), you will have to change the hardware configuration customization. The `sd_card_t` now points to new object that specifies the configuration of either an SPI interface or an SDIO interface,
and a new `type` member that identifies which type of interface.
Also, the `pcName` member of `sd_card_t` has been removed.
See [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration).

For example, if you were using a `hw_config.c` containing 
```C
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 17,    // The SPI slave select GPIO for this SD card
        // ...
```        
that would now become
```C
static sd_spi_if_t spi_ifs[] = {
    { 
        .spi = &spis[0],          // Pointer to the SPI driving this card
        .ss_gpio = 17,             // The SPI slave select GPIO for this SD card
        //...
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
        //...
```
### Migrating from no-OS-FatFS-SD-SPI-RPi-Pico [sdio](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/tree/sdio) branch
Instances of the interface classes `sd_spi_t` and `sd_sdio_t` are no longer embedded in `sd_card_t` as `spi_if` and `sdio_if`. They are moved to the top level as instances of `sd_spi_if_t` and `sd_sdio_if_t` and pointed to by instances of `sd_card_t`. 
Also, the `pcName` member of `sd_card_t` in the hardware configuration has been removed.
For example, if you were using a `hw_config.c` containing:
```C
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",  // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if.spi = &spis[0],  // Pointer to the SPI driving this card
        .spi_if.ss_gpio = 7,     // The SPI slave select GPIO for this SD card
        //...        
```
that would become:
```C
static sd_spi_if_t spi_ifs[] = {
    {
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 7,     // The SPI slave select GPIO for this SD card
        //...
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card     
        //...           
```
For details, see [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration). 

## Appendix C: Adding Additional Cards
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) additional Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).

Name|SPI0|GPIO|Pin |SPI|SDIO|MicroSD 0|MicroSD 1
----|----|----|----|---|----|---------|---------
CD1||14|19||||CD
CS1||15|20|SS or CS|DAT3||CS
MISO|RX|16|21|DO|DAT0|DO|DO
CS0||17|22|SS or CS|DAT3|CS|
SCK|SCK|18|24|SCLK|CLK|SCK|SCK
MOSI|TX|19|25|DI|CMD|DI|DI
CD0||22|29|||CD|
|||||||
GND|||18, 23|||GND|GND
3v3|||36|||3v3|3v3

### Wiring
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware
* [The hardware configuration](#customizing-for-the-hardware-configuration)
must be edited to add a new instance of 
[sd_card_t](#an-instance-of-sd_card_t-describes-the-configuration-of-one-sd-card-socket)
and its interface
[sd_sdio_if_t](#an-instance-of-sd_sdio_if_t-describes-the-configuration-of-one-sdio-to-sd-card-interface)
or
[sd_spi_if_t](#an-instance-of-sd_spi_if_t-describes-the-configuration-of-one-spi-to-sd-card-interface).
* Edit `ff14a/source/ffconf.h`. In particular, `FF_VOLUMES`:
```C
#define FF_VOLUMES		2
```


## Appendix D: Performance Tuning Tips
Obviously, if possible, use 4-bit SDIO instead of 1-bit SPI.
(See [Choosing the Interface Type(s)](#choosing-the-interface-types).)

Obviously, set the baud rate as high as you can. (See
[Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration)).

Consider increasing the system clock frequency (clk_sys).

If you are using SPI, try SPI mode 3 (CPOL=1, CPHA=1) instead of 0 (CPOL=0, CPHA=0). (See
[SPI Controller Configuration](#spi-controller-configuration).) This could buy a 15% speed boost.

TL;DR: In general, it is much faster to transfer a given number of bytes in one large write (or read) 
than to transfer the same number of bytes in multiple smaller writes (or reads). 

One quick and easy way to speed up many applications is to take advantage of the buffering built into the C library for standard I/O streams.
(See 
[fopencookie—open a stream with custom callbacks](https://sourceware.org/newlib/libc.html#fopencookie) and 
[setvbuf—specify file or stream buffering](https://sourceware.org/newlib/libc.html#setvbuf)). 
The application would use [fprintf](https://sourceware.org/newlib/libc.html#sprintf) instead of 
[f_fprintf](http://elm-chan.org/fsw/ff/doc/printf.html), 
or 
[fwrite](https://sourceware.org/newlib/libc.html#fwrite) instead of 
[f_write](http://elm-chan.org/fsw/ff/doc/write.html),
for example.
If you are using SDIO, it is critically important for performance to use `setvbuf` 
to set the buffer to an `aligned` buffer. 
Also, the buffer should be a multiple of the SD block size, 512 bytes, in size.
For example:
```C
    static char vbuf[1024] __attribute__((aligned));
    int err = setvbuf(file_p, vbuf, _IOFBF, sizeof vbuf);
```
If you have a record-oriented application, 
and the records are multiples of 512 bytes in size,
you might not see a significant speedup.
However, if, for example, you are writing text files with 
no fixed record length, the speedup can be great.
See
[examples/stdio_buffering/](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/tree/master/examples/stdio_buffering).

Now, for the details:
The modern SD card is a block device, meaning that the smallest addressable unit is a a block (or "sector") of 512 bytes. So, it helps performance if your write size is a multiple of 512. If it isn't, partial block writes involve reading the existing block, modifying it in memory, and writing it back out. With all the space in SD cards these days, it can be well worth it to pad a record length to a multiple of 512.

Generally, flash memory has to be erased before it can be written, and the minimum erase size is the "allocation unit" or "segment":

> AU (Allocation Unit): is a physical boundary of the card and consists of one or more blocks and its
size depends on each card. The maximum AU size is defined for memory capacity. Furthermore AU
is the minimal unit in which the card guarantees its performance for devices which complies with
Speed Class Specification. The information about the size and the Speed Class are stored in the
SD Status.

> -- SD Card Association; Physical Layer Specification Version 3.01

There is a controller in each SD card running all kinds of internal processes. When an amount of data to be written is smaller than a segment, the segment is read, modified in memory, and then written again. SD cards use various strategies to speed this up. Most implement a "translation layer". For any I/O operation, a translation from virtual to physical address is carried out by the controller. If data inside a segment is to be overwritten, the translation layer remaps the virtual address of the segment to another erased physical address. The old physical segment is marked dirty and queued for an erase. Later, when it is erased, it can be reused. Usually, SD cards have a cache of one or more segments for increasing the performance of read and write operations. The SD card is a "black box": much of this is invisible to the user, except as revealed in the Card-Specific Data register (CSD), SD_STATUS, and the observable performance characteristics. So, the write times are far from deterministic.

The Allocation Unit is typically 4 MiB for a 16 or 32 GB card, for example. Of course, nobody is going to be using 4 MiB write buffers on a Pico, but the AU is still important. For good performance and wear tolerance, it is recommended that the "disk partition" be aligned to an AU boundary. [SD Memory Card Formatter](https://www.sdcard.org/downloads/formatter/) makes this happen. For my 16 GB card, it set "Partition Starting Offset	4,194,304 bytes". This accomplished by inserting "hidden sectors" between the actual start of the physical media and the start of the volume. Also, it might be helpful to have your write size be some factor of the segment size.

There are more variables at the file system level. The FAT "allocation unit" (not to be confused with the SD card "allocation unit"), also known as "cluster", is a unit of "disk" space allocation for files. These are identically sized small blocks of contiguous space that are indexed by the File Allocation Table. When the size of the allocation unit is 32768 bytes, a file with 100 bytes in size occupies 32768 bytes of disk space. The space efficiency of disk usage gets worse with increasing size of allocation unit, but, on the other hand, the read/write performance increases. Therefore the size of allocation unit is a trade-off between space efficiency and performance. This is something you can change by formatting the SD card. See 
[f_mkfs](http://elm-chan.org/fsw/ff/doc/mkfs.html)
and 
[Description of Default Cluster Sizes for FAT32 File System](https://support.microsoft.com/en-us/topic/description-of-default-cluster-sizes-for-fat32-file-system-905ea1b1-5c4e-a03f-3863-e4846a878d31). 
Again, there might be some advantage to making your write size be some factor or multiple of the FAT allocation unit.
The `info` command in [examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/command_line) reports the allocation unit.

[File fragmentation](https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Fragmentation) can lead to long access times. 
Fragmented files can result from multiple files being incrementally extended in an interleaved fashion. 
One commonly used trick is to use [f_lseek](http://elm-chan.org/fsw/ff/doc/lseek.html) to pre-allocate a file to its ultimate size before beginning to write to it. Even better, you can pre-allocate a contiguous file using [f_expand](http://elm-chan.org/fsw/ff/doc/expand.html). 
(Also, see [FAT Volume Image Creator](http://elm-chan.org/fsw/ff/res/mkfatimg.zip) (Pre-creating built-in FAT volume).)
Obviously, you will need some way to keep track of how much valid data is in the file. 
You could use a file header.
Alternatively, if the file contains text, you could write an End-Of-File (EOF) character. 
In DOS, this is the character 26, which is the Control-Z character.
Alternatively, if the file contains records, each record could contain a magic number or checksum, so you can easily tell when you've reached the end of the valid records.
(This might be an obvious choice if you're padding the record length to a multiple of 512 bytes.)

For SDIO-attached cards, alignment of the read or write buffer is quite important for performance.
This library uses DMA with `DMA_SIZE_32`, and the read and write addresses must always be aligned to the current transfer size,
i.e., four bytes.
(For example, you could specify that the buffer has [\_\_attribute\_\_ ((aligned (4))](https://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/Type-Attributes.html).)
If the buffer address is not aligned, the library copies each block into a temporary buffer that is aligned and then writes it out, one block at a time.
(The SPI driver uses `DMA_SIZE_8` so the alignment isn't important.)

For a logging type of application, opening and closing a file for each update is hugely inefficient,
but if you can afford the time it can be a good way to minimize data loss in the event
of an unexpected power loss or that kind of thing.
You can also try to find a middle ground by periodically closing
and reopening a file, or switching to a new file.
A well designed directory structure can act as a sort of
hierarchical database for rapid retrieval of records
distributed across many small files.

## Appendix E: Troubleshooting
* **Check your grounds!** Maybe add some more if you were skimpy with them. The Pico has six of them.
* Turn on `DBG_PRINTF`. (See [Messages](#messages).) For example, in `CMakeLists.txt`, 
  ```CMake
  add_compile_definitions(USE_PRINTF USE_DBG_PRINTF)
  ```
  You might see a clue in the messages.
* Power cycle the SD card. Once an SD card is in SPI mode, the only way to get it back to SD mode is to power cycle it.
At power up, an SD card's CS/DAT3 line has a 50 kΩ pull up enabled in the card, but it will be disabled if the card is initialized,
and it won't be enabled again until the card is power cycled.
* Try lowering the SPI or SDIO baud rate (e.g., in `hw_config.c`). This will also make it easier to use things like logic analyzers.
  * For SPI, this is in the
  [spi_t](#spi-controller-configuration) instance.
  * For SDIO, this is in the 
  [sd_sdio_if_t](#an-instance-of-sd_sdio_if_t-describes-the-configuration-of-one-sdio-to-sd-card-interface) instance.
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk, PNY, and  Silicon Power.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```C
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
* Better yet, go to somwhere like [JLCPCB](https://jlcpcb.com/) and get a printed circuit board!

[^1]: as of [Pull Request #12 Dynamic configuration](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/12) (in response to [Issue #11 Configurable GPIO pins](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/11)), Sep 11, 2021

[^2]: as of [Pull Request #5 Bug in ff_getcwd when FF_VOLUMES < 2](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/5), Aug 13, 2021

[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high."
However, the Adafruit card has it's own 47 kΩ pull up on CS - Card Detect / Data Line [Bit 3], rendering it useless for Card Detection.

[^4]: [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)

[^5]: Rationale: Instances of `sd_spi_if_t` or `sd_sdio_if_t` are separate objects instead of being embedded in `sd_card_t` objects because `sd_sdio_if_t` carries a lot of state information with it (including things like data buffers). The union of the two types has the size of the largest type, which would result in a lot of wasted space in instances of `sd_spi_if_t`. I had another solution using `malloc`, but some people are frightened of `malloc` in embedded systems.

[^6]: *SPI Mode* in [How to Use MMC/SDC](http://elm-chan.org/docs/mmc/mmc_e.html#spimode)
