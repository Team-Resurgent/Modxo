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

////////////////////////////   LOG SECTION   //////////////////////////
#include <modxo/lpc_regs.h>
#include <string.h>

void lpc_reg_write(REG_T* reg, uint8_t data){
	if(reg->write_buffer != NULL && reg->value != NULL){
		if(reg->cread != 0)
			lpc_reg_reset(reg);

		reg->write_buffer[reg->cwrite++] = data;

		if(reg->cwrite >= reg->length){
			memcpy(reg->value, reg->write_buffer, reg->length);
			reg->cwrite = 0;
		}
	}
}

uint8_t lpc_reg_read(REG_T* reg){
	uint8_t data = 0;
	if(reg->read_buffer != NULL && reg->value != NULL && reg->cread < reg->length){

		if(reg->cwrite != 0)
			lpc_reg_reset(reg);

		if(reg->cread == 0){
			memcpy(reg->read_buffer, reg->value, reg->length);
		}

		data = reg->read_buffer[reg->cread++];
	}

	return data;
}

void lpc_reg_reset(REG_T* reg){
	reg->cread = 0;
	reg->cwrite = 0;
}
/////////////////////////////////////////////////////////////////////