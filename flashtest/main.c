/*
 * flashtest - Read the external QSPI flash chip's JEDEC ID and report it over USB.
 *
 * This is a small standalone Pico SDK app. On boot it reads the flash chip's
 * JEDEC ID (0x9F) and unique ID (0x4B) using the bootrom flash helpers, then
 * waits for a USB serial (CDC) connection and prints the results. It keeps
 * re-printing every couple of seconds so you always see output no matter when
 * you open the terminal.
 *
 * The JEDEC-ID read is the same technique used by Modxo's src/main.c
 * (get_flash_spi_clkdiv): send command 0x9F with flash_do_cmd() and read back
 * the manufacturer id followed by the two device-id bytes.
 *
 * SPDX short identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

// JEDEC "Read Identification" (RDID) command.
#define FLASH_CMD_JEDEC_ID 0x9F

// Number of bytes to clock out/in for the JEDEC read. Byte 0 is sent as the
// command and the response is shifted in behind it, so we need at least 4
// bytes: [cmd][manuf][dev1][dev2].
#define RDID_BUF_SIZE 8

typedef struct {
    uint8_t manuf_id;   // JEDEC manufacturer id
    uint8_t device_id1; // memory type
    uint8_t device_id2; // capacity (usually log2 of size in bytes)
    uint8_t unique_id[FLASH_UNIQUE_ID_SIZE_BYTES];
    bool    valid;
} flash_info_t;

// Best-effort manufacturer name from the JEDEC manufacturer id.
static const char *manufacturer_name(uint8_t id) {
    switch (id) {
        case 0xEF: return "Winbond";
        case 0xC8: return "GigaDevice";
        case 0xC2: return "Macronix";
        case 0x20: return "Micron/Numonyx";
        case 0x1F: return "Adesto/Atmel";
        case 0xBF: return "SST/Microchip";
        case 0x9D: return "ISSI";
        case 0x85: return "Puya";
        case 0xA1: return "Fudan";
        case 0x0B: return "XTX";
        case 0x68: return "BoHong/BYTe";
        case 0x5E: return "Zbit";
        case 0xEB: return "XMC";
        case 0x51: return "GigaDevice(GD25)";
        default:   return "Unknown";
    }
}

// Human-readable capacity. For most SPI-NOR chips the third JEDEC byte is the
// base-2 log of the density in bytes (e.g. 0x15 -> 2^21 = 2 MB).
static void capacity_string(uint8_t device_id2, char *out, size_t out_len) {
    if (device_id2 >= 10 && device_id2 <= 40) {
        uint64_t bytes = (uint64_t)1u << device_id2;
        if (bytes >= (1u << 20)) {
            snprintf(out, out_len, "%llu MB (2^%u bytes)",
                     (unsigned long long)(bytes >> 20), device_id2);
        } else {
            snprintf(out, out_len, "%llu KB (2^%u bytes)",
                     (unsigned long long)(bytes >> 10), device_id2);
        }
    } else {
        snprintf(out, out_len, "unknown");
    }
}

// Read the JEDEC id and unique id from the flash chip.
//
// flash_do_cmd() (and flash_get_unique_id, which calls it internally) briefly
// takes the flash out of XIP execute-in-place mode. While that is happening no
// other code -- including interrupt handlers -- may run from flash, or the core
// will fault. We therefore disable interrupts around these calls. This mirrors
// how the Modxo firmware reads the id very early in main() before anything else
// is running.
static void read_flash_info(flash_info_t *info) {
    uint8_t txbuf[RDID_BUF_SIZE] = { FLASH_CMD_JEDEC_ID };
    uint8_t rxbuf[RDID_BUF_SIZE] = { 0 };

    uint32_t ints = save_and_disable_interrupts();
    flash_do_cmd(txbuf, rxbuf, RDID_BUF_SIZE);
    flash_get_unique_id(info->unique_id);
    restore_interrupts(ints);

    // rxbuf[0] is clocked out while the command byte is still going out, so it
    // holds garbage; the real id starts at rxbuf[1].
    info->manuf_id   = rxbuf[1];
    info->device_id1 = rxbuf[2];
    info->device_id2 = rxbuf[3];

    // A missing/held bus reads as all 0x00 or all 0xFF -- treat those as invalid.
    info->valid = !((info->manuf_id == 0x00 && info->device_id1 == 0x00 && info->device_id2 == 0x00) ||
                    (info->manuf_id == 0xFF && info->device_id1 == 0xFF && info->device_id2 == 0xFF));
}

static void print_flash_info(const flash_info_t *info) {
    char cap[48];
    capacity_string(info->device_id2, cap, sizeof(cap));

    printf("==================== Flash Chip ID ====================\n");
    if (!info->valid) {
        printf("  No valid response from flash (read 0x%02X 0x%02X 0x%02X).\n",
               info->manuf_id, info->device_id1, info->device_id2);
        printf("=======================================================\n\n");
        return;
    }

    printf("  Manufacturer : 0x%02X (%s)\n", info->manuf_id, manufacturer_name(info->manuf_id));
    printf("  Memory type  : 0x%02X\n", info->device_id1);
    printf("  Capacity code: 0x%02X\n", info->device_id2);
    printf("  JEDEC ID     : %02X %02X %02X\n", info->manuf_id, info->device_id1, info->device_id2);
    printf("  Density      : %s\n", cap);

    printf("  Unique ID    : ");
    for (int i = 0; i < FLASH_UNIQUE_ID_SIZE_BYTES; i++) {
        printf("%02X", info->unique_id[i]);
    }
    printf("\n");
    printf("=======================================================\n\n");
}

int main(void) {
    // Bring up stdio. USB CDC is enabled in CMakeLists; UART is disabled.
    stdio_init_all();

    // Read the id once, up front, while the system is quiet.
    flash_info_t info;
    read_flash_info(&info);

    // Wait until a USB serial terminal actually opens, so the first report
    // isn't lost. If you're on a build without USB stdio this just returns.
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // Small settle delay after the host enumerates the port.
    sleep_ms(200);
    printf("\nflashtest: reading external QSPI flash JEDEC id...\n\n");

    // Print continuously so the info is visible whenever the terminal is opened.
    while (true) {
        print_flash_info(&info);
        sleep_ms(2000);
    }
}
