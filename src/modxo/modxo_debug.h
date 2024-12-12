/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx
*/
#pragma once

#ifdef DEBUG_PRINTS
#define DEBUG_SUPERIO_DISABLED
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) 
#endif
