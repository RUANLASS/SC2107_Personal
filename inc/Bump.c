// Bump.c
// Runs on MSP432
// Provide low-level functions that interface bump switches the robot.
// Daniel Valvano and Jonathan Valvano
// July 2, 2017

/* This example accompanies the books
   "Embedded Systems: Introduction to the MSP432 Microcontroller",
       ISBN: 978-1512185676, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Interfacing to the MSP432 Microcontroller",
       ISBN: 978-1514676585, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
       ISBN: 978-1466468863, , Jonathan Valvano, copyright (c) 2017
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/

Simplified BSD License (FreeBSD License)
Copyright (c) 2017, Jonathan Valvano, All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied, of the FreeBSD Project.
*/

#include <stdint.h>
#include "msp.h"

extern volatile uint8_t bump_data;
volatile uint32_t bump_count=0;

void Bump_Init(void){

    // write this as part of Lab 3
    P4->SEL0 &= ~0xED; P4->SEL1 &= ~0xED;
    P4->DIR &= ~(0xED);
    P4->REN |= 0xED;
    P4->OUT |= 0xED;
//    P4->IES |=  0xED;        // trigger on falling edge (switch pressed: 1 -> 0)
//    P4->IFG &= ~0xED;        // clear any flags already set
//    P4->IE  |=  0xED;        // enable Port 4 interrupts on the bump pins
//    NVIC->IP[38] = 2 << 5;   // Port 4 = IRQ38, priority 2
//    NVIC->ISER[1] = 1 << 6;  // IRQ38 -> ISER1 bit 6 (38 - 32 = 6)
}

uint8_t Bump_Read(void){

    uint8_t in = ~(P4->IN) & 0xED;
    return (in & 0x01) | ((in>>1) & 0x06) | ((in>>2) & 0x38);

    // write this as part of Lab 3


}

//void PORT4_IRQHandler(void){
//   bump_count++;
//   bump_data = Bump_Read();
//   P4->IFG &= ~0xED;        // clear flags, or this ISR runs forever
//}
