# SD card LPC protocol (example)

The Xbox side talks to the Pico over the Modxo LPC decode window. For the SD module, two 8-bit I/O ports are used (names and values match `sdcard.c`):

| Register | Address | Host write | Host read |
|----------|---------|------------|-----------|
| `MODXO_REGISTER_SDCARD_COMMAND` | `0xDEB0` | Selects the current opcode for the next data access | - |
| `MODXO_REGISTER_SDCARD_DATA` | `0xDEB1` | Parameter byte (depends on command) | Response byte (depends on command) |

The names below match `sdcard.h`. Pseudocode uses MSVC-style port I/O: `_outp(port, value)` and `_inp(port)` (typically `<conio.h>`); map those to your platform if needed (e.g. LPC bridge, MMIO).

---

## Directory listing (current folder)

The device keeps an internal **current working directory** path (FatFs style, e.g. `0:` or `0:/games`). **`SDCARD_COMMAND_REQUEST_DIR_LIST_REFRESH`** fills the cache from that folder. Use **`SDCARD_COMMAND_SET_LIST_ENTRY_INDEX`** plus **`REQUEST_CWD_ENTER_SELECTED`** to descend into a **directory** entry (the firmware appends its name to the internal path). **`REQUEST_CWD_PARENT`** goes up one level; **`REQUEST_CWD_TO_ROOT`** resets to volume root `0:`. After any CWD change, **refresh** again before using indices.

Flow:

1. **Refresh** the cached directory listing for the current folder.
2. Poll until the list is **ready**.
3. Read **count** (number of entries).
4. For each index `0 .. count-1`, select the entry, read **id**, **flags**, **name length**, then stream **name** bytes.

`SDCARD_LIST_FLAG_DIRECTORY` (`0x01`) in **flags** means the entry is a **directory**; if that bit is clear, treat it as a **file** (for this firmware, file reads only apply to non-directories).

### Pseudocode

```text
MODXO_REGISTER_SDCARD_COMMAND = 0xDEB0
MODXO_REGISTER_SDCARD_DATA    = 0xDEB1

; --- values from sdcard.h (include <sdcard.h> for these macros) ---
; SDCARD_COMMAND_REQUEST_DIR_LIST_REFRESH           = 1
; SDCARD_COMMAND_REQUEST_CWD_TO_ROOT               = 3
; SDCARD_COMMAND_REQUEST_CWD_PARENT                = 37
; SDCARD_COMMAND_REQUEST_CWD_ENTER_SELECTED        = 38
; SDCARD_COMMAND_SET_LIST_ENTRY_INDEX           = 32
; SDCARD_COMMAND_SET_LIST_NAME_INDEX            = 33
; SDCARD_COMMAND_GET_RESPONSE_LIST_READY        = 64
; SDCARD_COMMAND_GET_RESPONSE_LIST_COUNT        = 65
; SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_ID     = 66
; SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_FLAGS  = 67
; SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_LENGTH  = 68
; SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_BYTE    = 69
; SDCARD_COMMAND_GET_RESPONSE_CWD_RESULT             = 79
; SDCARD_CWD_RESULT_OK                               = 0
; SDCARD_CWD_RESULT_BAD_INDEX                        = 1
; SDCARD_CWD_RESULT_NOT_DIRECTORY                    = 2
; SDCARD_CWD_RESULT_PATH_TOO_LONG                    = 3
; SDCARD_CWD_RESULT_AT_ROOT                          = 4
; SDCARD_LIST_FLAG_DIRECTORY                         = 0x01

; CWD commands: one write to CMD each, synchronous (not queued). Then read result if needed.
function sdcard_cwd_to_root():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_REQUEST_CWD_TO_ROOT)

function sdcard_cwd_parent():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_REQUEST_CWD_PARENT)

; Uses current SET_LIST_ENTRY_INDEX; entry must be a directory (check flags first).
function sdcard_cwd_enter_selected():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_REQUEST_CWD_ENTER_SELECTED)

function sdcard_cwd_result() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_CWD_RESULT)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; Refresh directory listing (async on device core0) — one write to CMD queues the job
function sdcard_dir_list_refresh():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_REQUEST_DIR_LIST_REFRESH)

; After refresh, poll until ready
function sdcard_wait_dir_list_ready():
    loop:
        _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_READY)
        if (_inp(MODXO_REGISTER_SDCARD_DATA) != 0):
            break

; Number of entries (capped at 32 in firmware)
function sdcard_dir_list_count() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_COUNT)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; Select which entry (0 .. count-1) subsequent metadata/name reads use
function sdcard_dir_set_entry_index(byte index):
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_SET_LIST_ENTRY_INDEX)
    _outp(MODXO_REGISTER_SDCARD_DATA, index)

; Read id (same as index in current firmware)
function sdcard_dir_entry_id() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_ID)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; Bit 0 set => directory
function sdcard_dir_entry_flags() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_FLAGS)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

function sdcard_dir_entry_is_directory(flags) -> bool:
    return (flags & SDCARD_LIST_FLAG_DIRECTORY) != 0

; UTF-8 name length (excludes NUL)
function sdcard_dir_name_length() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_LENGTH)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; Stream name: reset offset, then read one byte per call (auto-advances)
function sdcard_dir_name_reset():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_SET_LIST_NAME_INDEX)
    _outp(MODXO_REGISTER_SDCARD_DATA, 0)

function sdcard_dir_name_next_byte() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_BYTE)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; --- Full example: build an array of { id, is_dir, name } ---
function fetch_sd_dir_entries() -> list of Entry:
    sdcard_dir_list_refresh()
    sdcard_wait_dir_list_ready()

    n = sdcard_dir_list_count()
    entries = empty list

    for i = 0 to n - 1:
        sdcard_dir_set_entry_index(i)
        id   = sdcard_dir_entry_id()
        flg  = sdcard_dir_entry_flags()
        is_dir = sdcard_dir_entry_is_directory(flg)
        nlen = sdcard_dir_name_length()

        sdcard_dir_name_reset()
        name = empty string or byte array
        for k = 0 to nlen - 1:
            append name, sdcard_dir_name_next_byte()

        entries.append(Entry(id: id, is_directory: is_dir, name: name))

    return entries
```

### Notes (directory listing)

- `SDCARD_COMMAND_REQUEST_DIR_LIST_REFRESH` is **queued by one write** to `MODXO_REGISTER_SDCARD_COMMAND`. A following write to `MODXO_REGISTER_SDCARD_DATA` (old two-step sequence) is ignored, so legacy hosts still enqueue only one refresh.
- Every **read** of `MODXO_REGISTER_SDCARD_DATA` uses whatever command was last written to `MODXO_REGISTER_SDCARD_COMMAND`; set the command byte **before** each data read (and before each data write that depends on the command).
- **`REQUEST_CWD_*`** update an internal path of up to **`SDCARD_CWD_PATH_MAX`** characters; they do **not** queue on the worker—run **`GET_RESPONSE_CWD_RESULT`** after an enter/parent if you need **`SDCARD_CWD_RESULT_*`**.
- Listing and file opens use that internal path; **`REQUEST_CWD_TO_ROOT`** sets it to **`0:`** (same as after boot). Refresh again if the card or tree may have changed.
- After a directory refresh, the device **closes** any prior file-read state; always refresh (or at least re-fetch count) before relying on indices.
- Entries **omit** regular files larger than **`SDCARD_FILE_MAX_SIZE`** (16 MiB); they do not appear in the count or indices. Directories are always listed.

---

## Reading file data (files in the listed folder, chunked)

Each `SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK` opens the file named by the **currently selected listing entry** under the device’s **internal CWD path**, reads up to **1024** bytes (`SDCARD_FILE_CHUNK_SIZE`) at file byte position **`chunk_index * SDCARD_FILE_CHUNK_SIZE`**, where **`chunk_index`** is a little-endian **`uint16`** set by `SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_LO` / `SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_HI`. Valid **`chunk_index`** values are **`0`..`max_chunk`** inclusive, where **`max_chunk = (file_size + SDCARD_FILE_CHUNK_SIZE - 1) / SDCARD_FILE_CHUNK_SIZE`** using **`file_size`** from the last directory refresh for the selected entry (same as **`GET_RESPONSE_FILE_SIZE_*`**): index **`max_chunk`** is allowed for an empty past-EOF read; larger indices return **`SDCARD_FILE_READ_RESULT_ERROR`** without reading. Queue the read with **one** write to the command port (no separate data byte for the request).

### Pseudocode

```text
; --- extra opcodes from sdcard.h ---
; SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK              = 2
; SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_LO             = 34
; SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_HI             = 35
; SDCARD_COMMAND_GET_RESPONSE_FILE_OP_READY         = 70
; SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_LO  = 71
; SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_HI  = 72
; SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_BYTE       = 73
; SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B0          = 74   ; LE, from listing refresh
; SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B1          = 75
; SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B2          = 76
; SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B3          = 77
; SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT      = 78
; SDCARD_FILE_READ_RESULT_OK                         = 1
; SDCARD_FILE_READ_RESULT_ERROR                      = 2
; SDCARD_FILE_READ_RESULT_IS_DIRECTORY               = 3
; SDCARD_FILE_READ_RESULT_BAD_INDEX                  = 4

function sdcard_file_set_chunk_index_le(uint16 chunk_index):
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_LO)
    _outp(MODXO_REGISTER_SDCARD_DATA, byte0(chunk_index))
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_SET_FILE_CHUNK_INDEX_HI)
    _outp(MODXO_REGISTER_SDCARD_DATA, byte1(chunk_index))

function sdcard_file_request_read_chunk():
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK)

function sdcard_file_wait_op_ready():
    loop:
        _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_OP_READY)
        if (_inp(MODXO_REGISTER_SDCARD_DATA) != 0):
            break

function sdcard_file_read_result() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

function sdcard_file_chunk_length() -> uint16:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_LO)
    lo = _inp(MODXO_REGISTER_SDCARD_DATA)
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_HI)
    hi = _inp(MODXO_REGISTER_SDCARD_DATA)
    return lo | (hi << 8)

function sdcard_file_chunk_next_byte() -> byte:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_BYTE)
    return _inp(MODXO_REGISTER_SDCARD_DATA)

; Total size of current listing entry (from last refresh); optional before streaming
function sdcard_file_entry_size_le() -> uint32:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B0)
    b0 = _inp(MODXO_REGISTER_SDCARD_DATA)
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B1)
    b1 = _inp(MODXO_REGISTER_SDCARD_DATA)
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B2)
    b2 = _inp(MODXO_REGISTER_SDCARD_DATA)
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B3)
    b3 = _inp(MODXO_REGISTER_SDCARD_DATA)
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24)

; Read one chunk for listing index (non-directory) in the current folder. Returns bytes read (0..1024) or signals failure via result.
function sdcard_read_file_chunk(byte entry_index, uint16 chunk_index, buffer) -> uint16:
    _outp(MODXO_REGISTER_SDCARD_COMMAND, SDCARD_COMMAND_SET_LIST_ENTRY_INDEX)
    _outp(MODXO_REGISTER_SDCARD_DATA, entry_index)
    sdcard_file_set_chunk_index_le(chunk_index)
    sdcard_file_request_read_chunk()
    sdcard_file_wait_op_ready()

    result = sdcard_file_read_result()
    if (result != SDCARD_FILE_READ_RESULT_OK):
        return 0

    n = sdcard_file_chunk_length()
    for i = 0 to n - 1:
        buffer[i] = sdcard_file_chunk_next_byte()
    return n

; Example: stream whole file in 1 KiB steps (caller skips directories)
function read_listing_file_into_buffer(byte entry_index, dest, uint32 max_len) -> uint32:
    total = 0
    chunk_ix = 0
    loop:
        n = sdcard_read_file_chunk(entry_index, chunk_ix, dest + total)
        if (n == 0):
            break
        total = total + n
        chunk_ix = chunk_ix + 1
        if (total >= max_len):
            break
        if (n < 1024):
            break
    return total
```

### Notes (file read)

- Always read **`SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT`** after the op is ready: **`SDCARD_FILE_READ_RESULT_OK`** is the normal success code even when **`chunk_length` is 0** (empty file, or **chunk index** equal to **`max_chunk`** for a past-EOF read). **`chunk_index > max_chunk`** (see intro) yields **`SDCARD_FILE_READ_RESULT_ERROR`** without opening the file.
- **`SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_BYTE`** advances an internal index; the firmware resets that index when each chunk job completes, so read chunk bytes **in order** without skipping.
- **`GET_RESPONSE_FILE_SIZE_B0`..`B3`** reflect the **currently selected listing entry** from the last directory refresh, not the live length on disk after refresh.
- File paths are built from the internal **CWD** plus the entry name; change folders with **`REQUEST_CWD_*`** as in the listing section, then refresh and select an entry before **`REQUEST_FILE_READ_CHUNK`**.
- Directories are rejected with **`SDCARD_FILE_READ_RESULT_IS_DIRECTORY`**; use directory listing **flags** first.
