# SD card LPC protocol (example)

The Xbox side talks to the Pico over the Modxo LPC decode window. For the SD module, two 8-bit registers are used (see `modxo/modxo_ports.h`):

| Register | Address | Host write | Host read |
|----------|---------|------------|-----------|
| Command  | `0xDEBE` | Selects the current opcode for the next data access | - |
| Data     | `0xDEBF` | Parameter byte (depends on command) | Response byte (depends on command) |

The names below match `sdcard.h`. Pseudocode uses `write_port(addr, value)` / `read_port(addr)`; map those to your platform (e.g. LPC bridge, MMIO, etc.).

---

## Root directory listing (names + file vs directory)

Flow:

1. **Refresh** the cached root list (`"0:/"`) on the device.
2. Poll until the list is **ready**.
3. Read **count** (number of entries).
4. For each index `0 .. count-1`, select the entry, read **id**, **flags**, **name length**, then stream **name** bytes.

`SDCARD_ROOT_FLAG_DIRECTORY` (`0x01`) in **flags** means the entry is a **directory**; if that bit is clear, treat it as a **file** (for this firmware, file reads only apply to non-directories).

### Pseudocode

```text
SDCARD_CMD   = 0xDEBE
SDCARD_DATA  = 0xDEBF

; --- values from sdcard.h (include <sdcard.h> for these macros) ---
; SDCARD_COMMAND_REQUEST_ROOT_LIST_REFRESH           = 1
; SDCARD_COMMAND_SET_ROOT_LIST_ENTRY_INDEX           = 32
; SDCARD_COMMAND_SET_ROOT_LIST_NAME_INDEX            = 33
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_READY        = 64
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_COUNT        = 65
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_ID     = 66
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_FLAGS  = 67
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_LENGTH  = 68
; SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_BYTE    = 69
; SDCARD_ROOT_FLAG_DIRECTORY                         = 0x01

; Refresh root listing (async on device core0) — one write to CMD queues the job
function sdcard_root_list_refresh():
    write_port(SDCARD_CMD, SDCARD_COMMAND_REQUEST_ROOT_LIST_REFRESH)

; After refresh, poll until ready
function sdcard_wait_root_list_ready():
    loop:
        write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_READY)
        if read_port(SDCARD_DATA) != 0:
            break

; Number of entries (capped at 32 in firmware)
function sdcard_root_list_count() -> byte:
    write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_COUNT)
    return read_port(SDCARD_DATA)

; Select which entry (0 .. count-1) subsequent metadata/name reads use
function sdcard_root_set_entry_index(byte index):
    write_port(SDCARD_CMD, SDCARD_COMMAND_SET_ROOT_LIST_ENTRY_INDEX)
    write_port(SDCARD_DATA, index)

; Read id (same as index in current firmware)
function sdcard_root_entry_id() -> byte:
    write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_ID)
    return read_port(SDCARD_DATA)

; Bit 0 set => directory
function sdcard_root_entry_flags() -> byte:
    write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_FLAGS)
    return read_port(SDCARD_DATA)

function sdcard_entry_is_directory(flags) -> bool:
    return (flags & SDCARD_ROOT_FLAG_DIRECTORY) != 0

; UTF-8 name length (excludes NUL)
function sdcard_root_name_length() -> byte:
    write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_LENGTH)
    return read_port(SDCARD_DATA)

; Stream name: reset offset, then read one byte per call (auto-advances)
function sdcard_root_name_reset():
    write_port(SDCARD_CMD, SDCARD_COMMAND_SET_ROOT_LIST_NAME_INDEX)
    write_port(SDCARD_DATA, 0)

function sdcard_root_name_next_byte() -> byte:
    write_port(SDCARD_CMD, SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_BYTE)
    return read_port(SDCARD_DATA)

; --- Full example: build an array of { id, is_dir, name } ---
function fetch_sd_root_entries() -> list of Entry:
    sdcard_root_list_refresh()
    sdcard_wait_root_list_ready()

    n = sdcard_root_list_count()
    entries = empty list

    for i = 0 to n - 1:
        sdcard_root_set_entry_index(i)
        id   = sdcard_root_entry_id()
        flg  = sdcard_root_entry_flags()
        is_dir = sdcard_entry_is_directory(flg)
        nlen = sdcard_root_name_length()

        sdcard_root_name_reset()
        name = empty string or byte array
        for k = 0 to nlen - 1:
            append name, sdcard_root_name_next_byte()

        entries.append(Entry(id: id, is_directory: is_dir, name: name))

    return entries
```

### Notes

- `SDCARD_COMMAND_REQUEST_ROOT_LIST_REFRESH` is **queued by one write** to `SDCARD_CMD`. A following write to `SDCARD_DATA` (old two-step sequence) is ignored, so legacy hosts still enqueue only one refresh.
- Every **read** of `SDCARD_DATA` uses whatever command was last written to `SDCARD_CMD`; set the command byte **before** each data read (and before each data write that depends on the command).
- Listing reflects the **root** of volume `0:` only; refresh again if the card or files may have changed.
- After a root refresh, the device **closes** any prior file-read state; always refresh (or at least re-fetch count) before relying on indices.

For **reading file contents** in 1 KiB chunks, see `SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK`, `SDCARD_COMMAND_SET_FILE_OFFSET_B0` through `SDCARD_COMMAND_SET_FILE_OFFSET_B3`, and the `SDCARD_COMMAND_GET_RESPONSE_FILE_*` opcodes in `sdcard.h`. Select the root entry with `SDCARD_COMMAND_SET_ROOT_LIST_ENTRY_INDEX` (data = index), set the byte offset, then a **single** write of `SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK` to the command port queues the read (the old data-byte index step is ignored).
