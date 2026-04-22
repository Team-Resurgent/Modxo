#pragma once

#include <stdint.h>
//
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool logger_enabled;
extern const uint32_t period;
extern absolute_time_t next_log_time;

void process_stdio(int cRxedChar);

#ifdef __cplusplus
}
#endif
