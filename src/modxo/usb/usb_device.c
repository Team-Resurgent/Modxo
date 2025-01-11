#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bsp/board_api.h"
#include "tusb.h"

void usb_init()
{
  board_init();
  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);
  
  /*if (board_init_after_tusb) {
    board_init_after_tusb();
  }*/
}

void usb_poll()
{
    tud_task(); // tinyusb device task
    //cdc_task();
}

bool usb_isconnected(void){
  return tud_connected();
}
