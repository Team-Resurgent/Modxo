#include <modxo/lpc_interface.h>
#include <flashrom.h>

#define TOTAL_IO_HANDLERS 32

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
    if (io_handler_count >= TOTAL_IO_HANDLERS)
        return false;

    hdlr_table[io_handler_count].port_base = port_base;
    hdlr_table[io_handler_count].last_port = port_base + total_ports;
    hdlr_table[io_handler_count].io_read   = read_cback;
    hdlr_table[io_handler_count].io_write  = write_cback;

    io_handler_count++;
}

static bool mem_subscriber(uint32_t port_base, size_t mem_size, lpc_mem_read_t read_cback, lpc_mem_write_t write_cback)
{
    // This module does not support memory subscription
    return false;
}

lpc_op_mapper_t lpc_op_mapper = {
    .mem_read       = flashrom_memread_handler,
    .mem_write      = flashrom_memwrite_handler,
    .io_read        = io_read,
    .io_write       = io_write,
    .io_subscriber  = io_subscriber,
    .mem_subscriber = mem_subscriber,
};