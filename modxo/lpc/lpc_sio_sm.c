#include <stdint.h>
#include <stdbool.h>
#include "hardware/regs/sio.h"
#include "modxo/lpc_interface.h" 

static uint32_t inputs;
register io_wo_32 gpio_oe_set asm("r4");
register io_wo_32 gpio_oe_clr asm("r5");
register io_wo_32 gpio_set asm("r6");
register io_wo_32 gpio_clr asm("r7");
register io_wo_32 gpio_in asm("r8");
register io_wo_32 gpio_out asm("r9");
// Lowlevel SIO Access

inline static void cancel_lframe(void)
{
    gpio_oe_set = 0x20; //Sets lframe as output (output is tied to 0)
}

inline static void restore_lframe(void)
{
    gpio_oe_clr = 0x20; //Sets lframe as output (output is tied to 0)
}


inline static bool get_clk(void)
{
    return (inputs&0x10);
}

inline static uint8_t get_lad(void)
{
    return inputs&0xF;
}

//Read Lad and clock bits (Requires official pinout for now GPIO0-5)
inline static void read_bus(void)
{
    inputs = gpio_in & 0x1F;
}

//Writes only LAD pins
inline static void write_bus(uint32_t v)
{
    uint32_t clear_bits = (~v)&0xF;
    gpio_set = v;
    gpio_clr = clear_bits;
}

//Set output enable HIGH? for output
inline static void set_lad_as_outputs(void)
{
    gpio_oe_set = 0xF;
}

//Set output enable LOW? for input
inline static void set_lad_as_inputs(void)
{
    gpio_oe_clr = 0xF;
}

inline static void wait_rising_edge(void)
{   
    do
        read_bus();
    while(inputs&0x10 != 0);

    while(inputs&0x10 == 0)
        read_bus();
}

inline static void wait_falling_edge(void)
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
        if(is_mem_op())
        {
            req.cycle_repeat = 8;
            cancel_lframe();
        }
        else
        {
            req.cycle_repeat = 4;
        }

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
    restore_lframe();
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



// Interface, init, loop and register callbacks
void lpc_sio_sm_init(void)
{
    gpio_oe_set = sio_hw->gpio_oe_set;
    gpio_oe_clr = sio_hw->gpio_oe_clr;
    gpio_in = sio_hw->gpio_in;
    gpio_out = sio_hw->gpio_out;
}

void lpc_sio_sm_main_loop(void)
{
    lpc_state = START;

    while(true)
        lpc_handlers[lpc_state]();
}

void lpc_interface_set_callback(LPC_OP_TYPE op, lpc_handler_cback cback)
{
    op_handlers[op] = cback;
}