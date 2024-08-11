/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
	uint32_t total_items;
	uint32_t item_size;
	int rear;
	int front;
	void *buffer;
} MODXO_QUEUE_T;

void modxo_queue_insert(MODXO_QUEUE_T *queue, void *item);
bool modxo_queue_remove(MODXO_QUEUE_T *queue, void *item);
bool modxo_queue_init(MODXO_QUEUE_T *queue, void *buffer, uint32_t item_size, uint32_t total_items);
