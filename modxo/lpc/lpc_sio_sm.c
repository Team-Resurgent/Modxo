enum State
{
    START,
    OP,
    MEM_ADDRESS,
    IO_ADDRESS,
    WRITE_VALUE,
    WRITE_TURN_AROUND,
    WRITE_WAIT,
    
    READ_TURN_AROUND,
    READ_WAIT,
    READ_SEND,

    FINAL_TURNAROUND,

    TOTAL_STATES
};




static void wait_start()
{

}

static void read_op()
{

}

static void read_mem_address()
{

}

static void read_io_address()
{
    
}

static void read_write_data()
{

}

static void turn_around()
{

}

static void send_wait()
{

}

static void send_read_data()
{

}




typedef uint8_t (*StateHandler)(uint8_t lad);
States[TOTAL_STATES]=
{
    [START]=wait_start,
    [OP]=read_op,
    [MEM_ADDRESS]=read_mem_address,
    [IO_ADDRESS]=read_io_address,
    [WRITE_VALUE]=read_data,
    [WRITE_TURN_AROUND]=turn_around,
    [WRITE_WAIT]=send_wait,
    [READ_TURN_AROUND]=,
    [READ_WAIT]=,
    [READ_SEND]=
}