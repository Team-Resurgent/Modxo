#include <stdint.h>
#include <stdbool.h>
#include "hardware/regs/sio.h"
#include "modxo/lpc_interface.h"

static uint16_t inputs;

// Lowlevel SIO Access
static inline void read_bus(void)
{
    inputs = sio_hw->gpio_in & 0x1F;
}

static inline void write_bus(uint16_t v)
{
    sio_hw->gpio_out = v;
}

static inline void set_lad_as_outputs()
{
    sio_hw->gpio_oe_clr = 0xF;
}

static inline void set_lad_as_inputs()
{
    sio_hw->gpio_oe_set = 0xF;
}

inline static void wait_rising_edge()
{   
    do
        read_bus();
    while(inputs&0x10 != 0);

    while(inputs&0x10 == 0)
        read_bus();
}

inline static void wait_falling_edge()
{
    do
    {
        read_bus();
    }while(inputs&0x10 == 0);

    while(inputs&0x10 != 0) read_bus();
}


typedef enum
{
    START,
    CYC_DIR,
    ADDRESS,
    WRITE_DATA,
    TAR_INPUT_END,
    TAR_OUTPUT_START,
    SYNC,
    READ_DATA,
    TAR_OUTPUT_END,
    TAR_INPUT_START,

    TOTAL_STATES

}LPC_States_t;

typedef enum {
    IO_READ   = 0,
    IO_WRITE  = 1,
    MEM_READ  = 2,
    MEM_WRITE = 3
}cyctype_t;

typedef struct {
    cyctype_t cyc;
    uint32_t  address;
    uint32_t  cycle_repeat;
    uint8_t   data;
    bool      syncing;
}lpc_req_t;

static LPC_States_t lpc_state;
static lpc_req_t req;

static bool operation_in_progress;
static bool start_operation;

lpc_handler_cback op_handlers[LPC_OP_TOTAL]=
{
    [LPC_OP_IO_READ]   = 0,
    [LPC_OP_IO_WRITE]  = 0,
    [LPC_OP_MEM_WRITE] = 0,
    [LPC_OP_MEM_READ]  = 0,
};

inline static void execute_operation(void)
{
    if(op_handlers[req.cyc])
        op_handlers[req.cyc](req.address, &req.data);
}


typedef void (*LPC_State_Handler) (void);

static inline bool is_mem_op(void)
{
    return (req.cyc & 0b10) == 0b10;
}

static inline bool is_write_dir()
{
    return (req.cyc & 0b1) == 0b1;
}

//State machine handlers
inline static void wait_start(void)
{
    wait_falling_edge();

    uint8_t lad = inputs&0xF;

    if( lad == 0)
        lpc_state = CYC_DIR;
}

inline static void get_cyctype_dir(void)
{
    wait_falling_edge();

    uint8_t lad = inputs&0xF;
    if(lad & 0b1000) //If cyc is not IO or MEM (bit3 == 1)
    {
        lpc_state = START;
    }
    else
    {
        req.cyc = (cyctype_t)(lad>>1);
        req.address = 0;
        req.cycle_repeat = is_mem_op() ? 8:4; //Memory operation addresses are 32bit (8 nibbles)
        lpc_state = ADDRESS;
    }
}

inline static void get_address(void)
{
    wait_falling_edge();


    uint8_t lad = inputs&0xF;
    req.address |= lad;
    req.cycle_repeat--;

    if(req.cycle_repeat == 0)
    {
        if(req.cyc & 0b1) //Write operation
        {
            lpc_state = WRITE_DATA;
            req.cycle_repeat = 2;
        }
        else
        {
            start_operation = true;
            lpc_state = TAR_INPUT_END;
        }
    }
    else
    {
        req.address <<=4;
    }
}

inline static void get_write_data(void)
{
    wait_falling_edge();


    uint8_t lad = inputs&0xF;
    req.data|=lad;
    req.cycle_repeat--;

    if(req.cycle_repeat == 0)
    {
        lpc_state = TAR_INPUT_END;
    }
    else
    {
        req.data<<=4;
    }
}

static void tar_input_end(void)
{
    wait_falling_edge();
    lpc_state = TAR_OUTPUT_START;
}

static void tar_output_start(void)
{
    wait_rising_edge();

    set_lad_as_outputs();
    write_bus(0b1111);

    lpc_state = SYNC;
}

static void sync(void)
{
    wait_rising_edge();
    write_bus(0x5);

    execute_operation(); //Execute it synchronously

    wait_rising_edge();
    write_bus(0x0);

    if(is_write_dir())
    {
        lpc_state = TAR_OUTPUT_END;
    }
    else
    {
        lpc_state = READ_DATA;
    }
}

static void send_read_data(void)
{
    wait_rising_edge();
    write_bus(req.data & 0b1111);
    req.data >>= 4;
    req.cycle_repeat--;

    if(req.cycle_repeat == 0)
    {
        lpc_state = TAR_OUTPUT_END;
    }
    else
    {
        req.data<<=4;
    }
}

static void tar_output_end(void)
{
    wait_rising_edge();
    lpc_state = TAR_INPUT_START;
    write_bus(0b1111);
    set_lad_as_inputs();
}

static void tar_input_start(void)
{
    wait_falling_edge();
    lpc_state = START;
}

static LPC_State_Handler lpc_handlers[TOTAL_STATES]=
{
    [START] = wait_start,
    [CYC_DIR] = get_cyctype_dir,
    [ADDRESS] = get_address,
    [WRITE_DATA] = get_write_data,
    [TAR_INPUT_END] = tar_input_end,       //TAR1
    [TAR_OUTPUT_START] = tar_output_start, //TAR1
    [SYNC] = sync,
    [READ_DATA] = send_read_data,
    [TAR_OUTPUT_END] = tar_output_end,      //TAR2
    [TAR_INPUT_START] = tar_input_start,    //TAR2
};

void lpc_sio_sm_init(void)
{
    lpc_state = START;
}

void lpc_sio_sm_main_loop(void)
{
    while(true)
        lpc_handlers[lpc_state]();
}

void lpc_interface_set_callback(LPC_OP_TYPE op, lpc_handler_cback cback)
{
    op_handlers[op] = cback;
}