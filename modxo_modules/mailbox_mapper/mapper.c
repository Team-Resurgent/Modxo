#include <modxo/lpc_interface.h>
#include <flashrom.h>
bool lpc_interface_mem_subscriber(uint32_t base_address, size_t mem_size, lpc_mem_read_t read_cback, lpc_mem_write_t write_cback);
uint8_t mem_read_hdlr(uint32_t address);
void mem_write_hdlr(uint32_t address, uint8_t data);

#define TOTAL_IO_HANDLERS 32

static lpc_io_read_t io_read_hdlr_table[0x10000];
static lpc_io_write_t io_write_hdlr_table[0x10000];

uint8_t io_read_hdlr(uint32_t address)
{
    return io_read_hdlr_table[address & 0xFFFF](address);
}

void io_write_hdlr(uint32_t address, uint8_t data)
{
    io_write_hdlr_table[address & 0xFFFF](address, data);
}

typedef struct
{
    uint16_t port_base;
    uint16_t last_port;
    lpc_io_read_t  io_read;
    lpc_io_write_t io_write;
}IO_HANDLER_T;

static IO_HANDLER_T hdlr_table[TOTAL_IO_HANDLERS];
static uint8_t io_handler_count = 0;

static void io_write(uint16_t address, uint8_t data)
{
    for (uint8_t tidx = 0; tidx < io_handler_count; tidx++)
    {
        if (hdlr_table[tidx].port_base < address && address < hdlr_table[tidx].last_port &&
            hdlr_table[tidx].io_write != NULL)
        {
            
            if(hdlr_table[tidx].io_read == NULL)
            {
                break;
            }

            hdlr_table[tidx].io_write(address, data);
        }
    }
}

static uint8_t io_read(uint16_t address)
{
    uint8_t data = 0xFF; // Default value

    for (uint8_t tidx = 0; tidx < io_handler_count; tidx++)
    {
        if (hdlr_table[tidx].port_base < address && address < hdlr_table[tidx].last_port)
        {
            if(hdlr_table[tidx].io_read == NULL)
            {
                break;
            }

            data = hdlr_table[tidx].io_read(address);
        }
    }

    return data;
}

static bool io_subscriber(uint16_t port_base, size_t total_ports, lpc_io_read_t read_cback, lpc_io_write_t write_cback)
{
    for(uint16_t port = port_base; port < port_base + total_ports; port++)
    {
        io_read_hdlr_table[port] = read_cback;
        io_write_hdlr_table[port] = write_cback;
    }
}

lpc_op_mapper_t lpc_op_mapper = {
    .mem_read       = mem_read_hdlr,
    .mem_write      = mem_write_hdlr,
    .io_read        = io_read,
    .io_write       = io_write,
    .io_subscriber  = io_subscriber,
    .mem_subscriber = lpc_interface_mem_subscriber,
};