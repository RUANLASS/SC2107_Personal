// Tachometer.c
// Runs on MSP432
// Provide mid-level functions that initialize ports, take
// angle and distance measurements, and report total travel
// statistics.
// Daniel Valvano
// May 30, 2017

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
#include "../inc/Clock.h"
#include "../inc/TA3InputCapture.h"
#include "msp.h"
#include "Tachometer.h"

uint16_t Tachometer_FirstRightTime, Tachometer_SecondRightTime;
uint16_t Tachometer_FirstLeftTime, Tachometer_SecondLeftTime;
int Tachometer_RightSteps = 0;     // incremented with every step forward; decremented with every step backward
int Tachometer_LeftSteps = 0;      // incremented with every step forward; decremented with every step backward
enum TachDirection Tachometer_RightDir = STOPPED;
enum TachDirection Tachometer_LeftDir = STOPPED;
// Lab5: edge counters that ignore direction (encoder B). Always +1 per edge.
volatile uint32_t Tachometer_RightEdges = 0, Tachometer_LeftEdges = 0;

void tachometerRightInt(uint16_t currenttime){
  Tachometer_RightEdges++;
  Tachometer_FirstRightTime = Tachometer_SecondRightTime;
  Tachometer_SecondRightTime = currenttime;
  if((P5->IN&0x01) == 0){   // right encoder B = P5.0
    // Encoder B is low, so this is a step backward
    Tachometer_RightSteps = Tachometer_RightSteps - 1;
    Tachometer_RightDir = REVERSE;
  }else{
    // Encoder B is high, so this is a step forward
    Tachometer_RightSteps = Tachometer_RightSteps + 1;
    Tachometer_RightDir = FORWARD;
  }
}

void tachometerLeftInt(uint16_t currenttime){
  Tachometer_LeftEdges++;
  Tachometer_FirstLeftTime = Tachometer_SecondLeftTime;
  Tachometer_SecondLeftTime = currenttime;
  if((P5->IN&0x04) == 0){   // left encoder B = P5.2
    // Encoder B is low, so this is a step backward
    Tachometer_LeftSteps = Tachometer_LeftSteps - 1;
    Tachometer_LeftDir = REVERSE;
  }else{
    // Encoder B is high, so this is a step backward
    Tachometer_LeftSteps = Tachometer_LeftSteps + 1;
    Tachometer_LeftDir = FORWARD;
  }
}

void Tachometer_Init(void){
  // RSLK MAX wiring: encoder B pins are P5.0 (right) and P5.2 (left).
  // Do NOT touch P9.2 - on the RSLK MAX it switches the odd reflectance IR LEDs.
  P5->SEL0 &= ~0x05;
  P5->SEL1 &= ~0x05;               // configure P5.0, P5.2 as GPIO
  P5->DIR &= ~0x05;                // make P5.0, P5.2 inputs
  TimerA3Capture_Init(&tachometerRightInt, &tachometerLeftInt);
}

void Tachometer_Get(uint16_t *leftTach, enum TachDirection *leftDir, int32_t *leftSteps,
                    uint16_t *rightTach, enum TachDirection *rightDir, int32_t *rightSteps){
  *leftTach = (Tachometer_SecondLeftTime - Tachometer_FirstLeftTime);
  *leftDir = Tachometer_LeftDir;
  *leftSteps = Tachometer_LeftSteps;
  *rightTach = (Tachometer_SecondRightTime - Tachometer_FirstRightTime);
  *rightDir = Tachometer_RightDir;
  *rightSteps = Tachometer_RightSteps;
}
