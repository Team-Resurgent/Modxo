// Handler for an externally connected psram chip
#include <hardware/address_mapped.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/regs/addressmap.h>
#include <hardware/structs/qmi.h>
#include <hardware/structs/xip_ctrl.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>
#include <stdio.h>

#include <string.h>
#include <modxo/lpc_mem_window.h>

#define ACCESS_REGION 0x10000000  //Cached
#define PSRAM_OFFSET  0x01000000  //Window 2 at 16MB (CSn1)

#define PSRAM_CMD_WIN_OFFSET 0
#define PSRAM_CMD_INIT 1
#define PSRAM_CMD_GET_SIZE 2
#define PSRAM_CMD_GET_ID 3
#define PSRAM_CMD_HAS_INITED 4

struct {
    uint8_t  mfg;
    uint8_t  kgd;
    uint8_t  density;
    uint8_t eid[6];
} psram_info;

// PSRAM SPI command codes
#define PSRAM_CMD_QUAD_END 0xF5
#define PSRAM_CMD_QUAD_ENABLE 0x35
#define PSRAM_CMD_READ_ID 0x9F
#define PSRAM_CMD_RSTEN 0x66
#define PSRAM_CMD_RST 0x99
#define PSRAM_CMD_QUAD_READ 0xEB
#define PSRAM_CMD_QUAD_WRITE 0x38
#define PSRAM_CMD_NOOP 0xFF

static void set_psram_timing(void);
static void psram_get_size(void);
static void psram_init(void);

static uint8_t *psram_address = (uint8_t*)(ACCESS_REGION+PSRAM_OFFSET); // XIP2 base address for PSRAM
static uint8_t psram_size = 0;  // Size in MiB (1, 2, 4, or 8 MiB)
static uint8_t psram_id = 0xFF; //Unitialized ID
static uint32_t psram_window_offsets[NUM_LPC_MEM_WINDOWS];
static bool psram_inited = false;


static uint32_t send_direct_cmd(uint8_t cmd, bool quad_mode)
{
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    // RESETEN, RESET and quad enable
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS| (quad_mode?QMI_DIRECT_TX_IWIDTH_VALUE_Q:QMI_DIRECT_TX_IWIDTH_VALUE_S)<<QMI_DIRECT_TX_IWIDTH_LSB  | QMI_DIRECT_TX_OE_BITS | cmd;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0);
    
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
}

static void psram_get_info(void)
{
    uint16_t val;
   // Read the id
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;

    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_DWIDTH_BITS | PSRAM_CMD_READ_ID;
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_DWIDTH_BITS;

    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0);
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0);

    qmi_hw->direct_tx = QMI_DIRECT_TX_DWIDTH_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_DWIDTH_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_DWIDTH_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_DWIDTH_BITS;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0);
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0);
    
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    val = (uint16_t)qmi_hw->direct_rx;
    psram_info.mfg= val&0xFF;
    psram_info.kgd= val>>8;

    val = (uint16_t)qmi_hw->direct_rx;
    psram_info.density = (val&0xFF)>>5;
    psram_info.eid[5]= (val&0x1F);
    psram_info.eid[4]= val>>8;

    val = (uint16_t)qmi_hw->direct_rx;
    psram_info.eid[3]= val&0xFF;
    psram_info.eid[2]= val>>8;

    val = (uint16_t)qmi_hw->direct_rx;
    psram_info.eid[1]= val&0xFF;
    psram_info.eid[0]= val>>8;
}

static void psram_configure()
{    
     qmi_hw->m[1].timing = QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Break between pages.
		                   3 << QMI_M1_TIMING_SELECT_HOLD_LSB | // Delay releasing CS for 3 extra system cycles.
		                   1 << QMI_M1_TIMING_COOLDOWN_LSB | 
                           1 << QMI_M1_TIMING_RXDELAY_LSB |
		                  16 << QMI_M1_TIMING_MAX_SELECT_LSB |  // In units of 64 system clock cycles. PSRAM says 8us max. 8 / 0.00752 /64
											                    // = 16.62
		                   7 << QMI_M1_TIMING_MIN_DESELECT_LSB | // In units of system clock cycles. PSRAM says 50ns.50 / 7.52 = 6.64
		                   4 << QMI_M1_TIMING_CLKDIV_LSB; 

    qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
                         QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
                         QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
                         QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB |
                         QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB);

    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ;

    qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
                         QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
                         QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
                         QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
                         QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
                         QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
                         QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB |
                         QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB);

    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE;

    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
    xip_ctrl_hw->ctrl &= ~(XIP_CTRL_NO_UNTRANSLATED_NONSEC_BITS | XIP_CTRL_EN_NONSECURE_BITS | XIP_CTRL_EN_SECURE_BITS);
}

static void psram_detect(void)
{
    uint32_t intr_stash = save_and_disable_interrupts();
    qmi_hw->direct_csr = QMI_DIRECT_CSR_EN_BITS;
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0);

    send_direct_cmd(PSRAM_CMD_QUAD_END, true);
    psram_get_info();
    send_direct_cmd(PSRAM_CMD_RSTEN, false);
    send_direct_cmd(PSRAM_CMD_RST, false);
    send_direct_cmd(PSRAM_CMD_QUAD_ENABLE, false);

    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_EN_BITS);
    psram_configure();
    
    restore_interrupts(intr_stash);
}


bool psram_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t psram_offset = psram_window_offsets[window_id];
	uint32_t psram_length = lpc_mem_windows[window_id].length;
    uint32_t offset = psram_offset + (addr - lpc_mem_windows[window_id].base_addr);

	*data = psram_address[offset];

	return true;
}

bool psram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t psram_offset = psram_window_offsets[window_id];
	uint32_t psram_length = lpc_mem_windows[window_id].length;
    uint32_t offset = psram_offset + (addr - lpc_mem_windows[window_id].base_addr);

	psram_address[offset] = *data;

	return true;
}

uint8_t psram_handler_control_peek(uint8_t cmd, uint8_t data) {
    uint8_t return_value = 0;
	switch(cmd) {
	case PSRAM_CMD_WIN_OFFSET:
		current_long_val = psram_window_offsets[current_window_id];
		break;
    case PSRAM_CMD_GET_SIZE:
        return_value = psram_size;
        break;
    case PSRAM_CMD_GET_ID:
        return_value = psram_id;
        break;
    case PSRAM_CMD_HAS_INITED:
        return_value = psram_inited;
        break;
	}

	return return_value;
}

uint8_t psram_handler_control_set(uint8_t cmd, uint8_t data) {
	switch(cmd) {
	case PSRAM_CMD_WIN_OFFSET:
		psram_window_offsets[current_window_id] = current_long_val;
		break;
    case PSRAM_CMD_INIT:
	    psram_handler_init();
        break;
	}

	return 0;
}

uint8_t psram_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? psram_handler_control_peek(cmd, data) : psram_handler_control_set(cmd, data);
}

void psram_handler_init() {
    if(psram_inited) return;

	// Initialize the psram
	// Set the PSRAM CS pin in the SDK
    gpio_set_function(PSRAM_CS1, GPIO_FUNC_XIP_CS1);

    // start with invlid values
    psram_size = 0;
    psram_id = 0xFF;

    psram_detect();
    
    psram_size = 2 << psram_info.density;
    psram_id = psram_info.kgd;

    psram_inited = true;
}

