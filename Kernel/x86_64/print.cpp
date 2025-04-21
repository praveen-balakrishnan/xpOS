/**
    Copyright 2023-2025 Praveen Balakrishnan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0
*/

#include <cstdint>

#include "Arch/IO/IO.h"
#include "print.h"

static constexpr int NUM_COLS = 80;
static constexpr int NUM_ROWS = 25;
static constexpr uint64_t KERNEL_BASE = 0xFFFFFF8000000000;
static constexpr uint64_t PHYS_VIDEOMEM_BASE = 0xB8000;
static volatile uint16_t* VideoMemory = (uint16_t*)(PHYS_VIDEOMEM_BASE+KERNEL_BASE);

static uint8_t x=0, y=0;
static constexpr uint16_t SerialPort = 0x3F8;

int is_transmit_empty() {
    return IO::in_8(SerialPort + 5) & 0x20;
 }
 
 void write_serial(char a) {
    while (is_transmit_empty() == 0);
 
    IO::out_8(SerialPort,a);
 }

void printf(const char* str)
{
    if (y >= NUM_ROWS)
        return;
    
    for(int i = 0; str[i] != '\0'; ++i) {
        write_serial(str[i]);

        switch(str[i]) {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[NUM_COLS*y+x] = (VideoMemory[NUM_COLS*y+x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if(x >= NUM_COLS) {
            x = 0;
            y++;
        }

        if(y >= NUM_ROWS) {
            for(y = 0; y < NUM_ROWS; y++)
                for(x = 0; x < NUM_COLS; x++)
                    VideoMemory[NUM_COLS*y+x] = (VideoMemory[NUM_COLS*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
            break;
        }
    }
}

void printf(uint64_t n)
{
    char buffer[50];
    uint64_t i = 0;

    uint64_t n1 = n;

    while(n1 != 0) {
        buffer[i++] = n1 % 10 + '0';
        n1=n1/10;
    }

    buffer[i] = '\0';

    for(uint64_t t = 0; t < i/2; t++) {
        buffer[t] ^= buffer[i-t-1];
        buffer[i-t-1] ^= buffer[t];
        buffer[t] ^= buffer[i-t-1];
    }

    if(n == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
    }   

    printf(buffer);
}


 

void clear_screen_and_init_serial()
{
    // Enable COM1.
    IO::out_8(SerialPort + 1, 0);
    IO::out_8(SerialPort + 3, 0x80);
    IO::out_8(SerialPort + 0, 0x03);
    IO::out_8(SerialPort + 1, 0x00);
    IO::out_8(SerialPort + 3, 0x03);
    IO::out_8(SerialPort + 2, 0xC7);
    IO::out_8(SerialPort + 4, 0x0B);
    IO::out_8(SerialPort + 4, 0x1E); 
    IO::out_8(SerialPort + 4, 0x0F);

    for (int i=0; i< NUM_COLS; i++) {
        for (int j=0; j < NUM_ROWS;j++)
            VideoMemory[NUM_COLS*j+i] = (VideoMemory[NUM_COLS*j+i] & 0xFF00) | ' ';
    }
}