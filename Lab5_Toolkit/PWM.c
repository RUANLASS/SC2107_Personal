// PWM.c
// Runs on MSP432
// PWM on P2.6 using TimerA0 TA0.CCR3
// PWM on P2.7 using TimerA0 TA0.CCR4
// Derived from msp432p401_portmap_01.c in MSPware
// Jonathan Valvano
// February 17, 2017

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

#include "msp.h"


void PWM_Init34(uint16_t period, uint16_t duty3, uint16_t duty4){

    if(duty3 >= period) return; // bad input
     if(duty4 >= period) return; // bad input
     P2->DIR |= 0xC0;          // P2.6, P2.7 output
     P2->SEL0 |= 0xC0;         // P2.6, P2.7 Timer0A functions
     P2->SEL1 &= ~0xC0;        // P2.6, P2.7 Timer0A functions
     TIMER_A0->CCTL[0] = 0x0080;      // CCI0 toggle
     TIMER_A0->CCR[0] = period;       // Period is 2*period*8*83.33ns is 1.333*period
     TIMER_A0->EX0 = 0x0000;        //    divide by 1
     TIMER_A0->CCTL[3] = 0x0040;      // CCR1 toggle/reset
     TIMER_A0->CCR[3] = duty3;        // CCR1 duty cycle is duty3/period
     TIMER_A0->CCTL[4] = 0x0040;      // CCR2 toggle/reset
     TIMER_A0->CCR[4] = duty4;        // CCR2 duty cycle is duty4/period
     TIMER_A0->CTL = 0x02F0;        // SMCLK=12MHz, divide by 8, up-down mode
   // bit  mode
   // 9-8  10    TASSEL, SMCLK=12MHz
   // 7-6  11    ID, divide by 8
   // 5-4  11    MC, up-down mode
   // 2    0     TACLR, no clear
   // 1    0     TAIE, no interrupt
   // 0          TAIFG
}


void PWM_Duty3(uint16_t duty3){

    if(duty3 >= TIMER_A0->CCR[0]) return; // bad input
    TIMER_A0->CCR[3] = duty3;        // CCR3 duty cycle is duty3/period
}


void PWM_Duty4(uint16_t duty4){

    if(duty4 >= TIMER_A0->CCR[0]) return; // bad input
    TIMER_A0->CCR[4] = duty4;        // CCR4 duty cycle is duty4/period
}


