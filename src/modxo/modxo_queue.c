/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string.h>
#include <pico/platform.h>
#include "modxo_queue.h"

static inline void *item_address(MODXO_QUEUE_T *queue, uint32_t item_n)
{
   uint32_t address = (uint32_t)queue->buffer;
   address += item_n * queue->item_size;
   return (void *)address;
}

void __not_in_flash_func(modxo_queue_insert)(MODXO_QUEUE_T *queue, void *item)
{
   queue->rear = (queue->rear + 1) % queue->total_items;
   if (queue->rear == queue->front)
   {
      queue->rear--;
   }
   else
   {
      void *address = item_address(queue, queue->rear);
      memcpy(address, item, queue->item_size);
   }
}

bool __not_in_flash_func(modxo_queue_remove)(MODXO_QUEUE_T *queue, void *item)
{

   if (queue->front != queue->rear)
   {
      void *address;
      queue->front = (queue->front + 1) % queue->total_items;
      address = item_address(queue, queue->front);
      memcpy(item, address, queue->item_size);
      return true;
   }

   return false;
}

bool __not_in_flash_func(modxo_queue_init)(MODXO_QUEUE_T *queue, void *buffer, uint32_t item_size, uint32_t total_items)
{
   bool retv = false;

   if (queue != NULL)
   {
      queue->buffer = buffer;
      queue->item_size = item_size;
      queue->total_items = total_items;
      retv = true;
   }

   return retv;
}