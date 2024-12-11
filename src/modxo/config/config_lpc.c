#include "config_lpc.h"
#include "config_nvm.h"

#include "../modxo_ports.h"
#include "../lpc/lpc_interface.h"

static NVM_REGISTER_SEL reg_sel = NVM_REGISTER_NONE;

void config_set_reg_sel( NVM_REGISTER_SEL reg){
    reg_sel = reg;
}

NVM_REGISTER_SEL config_get_reg_sel(void){
    return reg_sel;
}

void config_set_value(uint8_t value){
    bool save = false;

    switch(reg_sel){
        case NVM_REGISTER_DISPLAY_INTERFACE:
            /*if(config.display_config.interface != value)
                save = true;

            display_set_interface(value);
            break;*/
        case NVM_REGISTER_DISPLAY1_ADDRESS:
            if(config.display_config.addr1 != value)
                save = true;

            config.display_config.addr1 = value;
            break;
        case NVM_REGISTER_DISPLAY2_ADDRESS:
            if(config.display_config.addr2 != value)
                save = true;

            config.display_config.addr2 = value;
            break;
        case NVM_REGISTER_ENABLE_SUPERIO_SP:
            if(config.enable_superio_sp != value)
                save = true;
            config.enable_superio_sp = value;
            break;
        case NVM_REGISTER_BOOT_BANK_SIZE:
            if(config.first_bootbank_size != value)
                save = true;

            config.first_bootbank_size = value;
            break;
        case NVM_REGISTER_RGB_STATUS_PF:
            if(config.rgb_status_pf != value)
                save = true;

            config.rgb_status_pf = value;
            break;
        case NVM_REGISTER_RGB_STRIP1_PF:
            if(config.rgb_strip_pf[0] != value)
                save = true;

            config.rgb_strip_pf[0] = value;
            break;
        case NVM_REGISTER_RGB_STRIP2_PF:
            if(config.rgb_strip_pf[1] != value)
                save = true;

            config.rgb_strip_pf[1] = value;
            break;
        case NVM_REGISTER_RGB_STRIP3_PF:
            if(config.rgb_strip_pf[2] != value)
                save = true;

            config.rgb_strip_pf[2] = value;
            break;
        case NVM_REGISTER_RGB_STRIP4_PF:
            if(config.rgb_strip_pf[3] != value)
                save = true;

            config.rgb_strip_pf[3] = value;
            break;
        default:
            break;
    }

    if(save){
        config_save_parameters();
    }
}

uint8_t config_get_value(void){
    uint8_t value=0;
    switch(reg_sel){
        case NVM_REGISTER_DISPLAY_INTERFACE:
            value = config.display_config.interface;
            break;
        case NVM_REGISTER_DISPLAY1_ADDRESS:
            value = config.display_config.addr1;
            break;
        case NVM_REGISTER_DISPLAY2_ADDRESS:
            value = config.display_config.addr2;
            break;
        case NVM_REGISTER_ENABLE_SUPERIO_SP:
            value = config.enable_superio_sp;
            break;
        case NVM_REGISTER_BOOT_BANK_SIZE:
            value = config.first_bootbank_size;
            break;
        case NVM_REGISTER_RGB_STATUS_PF:
            value = config.rgb_status_pf = value;
            break;
        case NVM_REGISTER_RGB_STRIP1_PF:
            value = config.rgb_strip_pf[0];
            break;
        case NVM_REGISTER_RGB_STRIP2_PF:
            value = config.rgb_strip_pf[1];
            break;
        case NVM_REGISTER_RGB_STRIP3_PF:
            value = config.rgb_strip_pf[2];
            break;
        case NVM_REGISTER_RGB_STRIP4_PF:
            value = config.rgb_strip_pf[3];
            break;
        default:
            break;
    }

    return value;
}


static void config_read_hdlr(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case MODXO_REGISTER_CONFIG_REG_SEL:
        *data = config_get_reg_sel();
        break;
    case MODXO_REGISTER_CONFIG_REG_VAL:
        *data = config_get_value();
        break;
    default:
        *data = 0;
    break;
    }
}

static void config_write_hdlr(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case MODXO_REGISTER_CONFIG_REG_SEL:
        config_set_reg_sel(*data);
        break;
    case MODXO_REGISTER_CONFIG_REG_VAL:
        config_set_value(*data);
        break;
    }
}

void config_nvm_init(void)
{
    config_retrieve_parameters();
    lpc_interface_add_io_handler(MODXO_REGISTER_CONFIG_REG_SEL, 0xFFFE, config_read_hdlr, config_write_hdlr);
}