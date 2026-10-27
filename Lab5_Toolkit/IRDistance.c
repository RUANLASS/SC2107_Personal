// IRDistance.c
// Runs on MSP432
// Provide mid-level functions that convert raw ADC
// values from the GP2Y0A21YK0F infrared distance sensors to
// distances in mm.
// Jonathan Valvano
// May 25, 2017

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

// Pololu #3543 Vreg (5V regulator output) connected to all three Pololu #136 GP2Y0A21YK0F Vcc's (+5V) and MSP432 +5V (J3.21)
// Pololu #3543 Vreg (5V regulator output) connected to positive side of three 10 uF capacitors physically near the sensors
// Pololu ground connected to all three Pololu #136 GP2Y0A21YK0F grounds and MSP432 ground (J3.22)
// Pololu ground connected to negative side of all three 10 uF capacitors
// MSP432 P9.0 (J5) (analog input to MSP432) connected to right GP2Y0A21YK0F Vout
// MSP432 P4.1 (J1.5) (analog input to MSP432) connected to center GP2Y0A21YK0F Vout
// MSP432 P9.1 (J5) (analog input to MSP432) connected to left GP2Y0A21YK0F Vout
#include <stdint.h>
#include "../inc/ADC14.h"
#include "msp.h"

// IR_USE_FIT 1: d(mm) = A/(n + B) + C  (PLUS B, so B values are NEGATIVE)
// IR_USE_FIT 0: piecewise-linear table
#define IR_USE_FIT     1
#define IR_FAR_MM      800

#if IR_USE_FIT
#define AL 1210780
#define BL (-502)
#define CL (-19)
#define AC 1189725
#define BC (-511)
#define CC 10
#define AR 1520439
#define BR (197)
#define CR (-31)

static int32_t IR_FitConvert(int32_t n, int32_t A, int32_t B, int32_t C){
  int32_t d;
  if(n + B <= 0) return IR_FAR_MM;
  d = A/(n + B) + C;
  if(d > IR_FAR_MM || d < 0) d = IR_FAR_MM;
  return d;
}

int32_t LeftConvert(int32_t nl){   return IR_FitConvert(nl, AL, BL, CL); }
int32_t CenterConvert(int32_t nc){ return IR_FitConvert(nc, AC, BC, CC); }
int32_t RightConvert(int32_t nr){  return IR_FitConvert(nr, AR, BR, CR); }

#else
typedef struct { int32_t adc; int32_t mm; } IRPoint;
static const IRPoint IRLeftTable[]   = { {11000,100}, {8500,150}, {6300,200}, {5600,250}, {4850,300} };
static const IRPoint IRCenterTable[] = { {11900,100}, {7850,150}, {6200,200}, {5500,250}, {4700,300} };
static const IRPoint IRRightTable[]  = { { 9600,100}, {7700,150}, {6100,200}, {5300,250}, {4600,300} };
#define NPTS(t) ((int)(sizeof(t)/sizeof((t)[0])))

static int32_t IR_TableConvert(int32_t n, const IRPoint *t, int npts){
  int i;
  if(n >= t[0].adc) return t[0].mm;
  if(n <= t[npts-1].adc) return IR_FAR_MM;
  for(i = 0; i < npts-1; i++){
    if(n <= t[i].adc && n > t[i+1].adc){
      return t[i].mm + ((t[i].adc - n)*(t[i+1].mm - t[i].mm))
                       /(t[i].adc - t[i+1].adc);
    }
  }
  return IR_FAR_MM;
}

int32_t LeftConvert(int32_t nl){   return IR_TableConvert(nl, IRLeftTable,   NPTS(IRLeftTable)); }
int32_t CenterConvert(int32_t nc){ return IR_TableConvert(nc, IRCenterTable, NPTS(IRCenterTable)); }
int32_t RightConvert(int32_t nr){  return IR_TableConvert(nr, IRRightTable,  NPTS(IRRightTable)); }
#endif
