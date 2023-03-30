/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"


// Blinking rate in milliseconds
#define BLINKING_RATE     1000ms
DigitalOut led(PA_5);
InterruptIn button(PC_13);
InterruptIn FC1(PB_1);
InterruptIn FC2(PB_2);

void button_Handler(void){
    led = !led;
}

void FC1_Handler(void){
    led = !led;
}

void FC2_Handler(void){
    led = !led;
}

int main()
{
    // Inerrupt functions

    FC1.rise(FC1_Handler);
    FC2.rise(FC2_Handler);
    button.rise(button_Handler);

    while (true) {
        led = !led;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}
