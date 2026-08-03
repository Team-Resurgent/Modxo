# flashtest

A small standalone Pico SDK app that reads the external QSPI flash chip's
**JEDEC ID** (manufacturer / memory type / capacity) and **unique ID**, then
prints them over USB serial once a terminal connects.

It reuses the same technique as Modxo's `src/main.c` (`get_flash_spi_clkdiv`):
send the JEDEC `0x9F` command with `flash_do_cmd()` and read back the id bytes.

## Build

This project uses the `pico-sdk` bundled with the Modxo repo at
`../external/pico-sdk`, so make sure the submodule is checked out first:

```
git submodule update --init --recursive
```

Then, from this `flashtest` folder:

```
mkdir build
cd build
cmake -G Ninja ..            # add -DPICO_BOARD=pico2 for an RP2350 / Pico 2
ninja
```

This produces `build/flashtest.uf2`.

## Run

1. Hold **BOOTSEL** while plugging the board in (it mounts as a USB drive).
2. Copy `flashtest.uf2` onto that drive. The board reboots and runs the app.
3. Open the board's USB serial port (e.g. `screen /dev/ttyACM0 115200`,
   or PuTTY / the VS Code serial monitor on Windows).

You should see something like:

```
==================== Flash Chip ID ====================
  Manufacturer : 0xEF (Winbond)
  Memory type  : 0x40
  Capacity code: 0x15
  JEDEC ID     : EF 40 15
  Density      : 2 MB (2^21 bytes)
  Unique ID    : E4681B3C0A2F9C57
=======================================================
```

The report repeats every 2 seconds so it's visible whenever you open the port.

## Notes

- `flash_do_cmd()` / `flash_get_unique_id()` briefly drop the flash out of XIP,
  so interrupts are disabled around them (nothing may execute from flash during
  that window). The id is read once at startup, then just re-printed.
- The manufacturer table and capacity decode are best-effort; unknown ids are
  still printed in raw hex so you can look them up.
