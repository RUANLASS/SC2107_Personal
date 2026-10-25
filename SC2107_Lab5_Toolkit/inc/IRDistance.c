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



// ---------------------------------------------------------------------------
// Lab5 rewrite (fixes from the Lab4 version):
//  1. The 100-200 mm segment used the 50-100 mm end points, so an obstacle at
//     200 mm read as ~290 mm. Each segment now uses its own two points.
//  2. Gap between 2824 and 2842 (typo) returned 0 mm.
//  3. Readings outside the table returned 0 mm. "0" looks like "touching" to
//     obstacle-avoidance code, so far readings now return IR_FAR_MM and very
//     close readings return the nearest table distance.
//  4. One table per sensor so each can be re-calibrated on its own.
//
// HOW TO RE-CALIBRATE (do this at the start of Lab5 - takes 5 minutes):
//  Run the Lab4 project (or Lab5_Toolkit TASK 0), put a box at each distance,
//  read nl / nc / nr (filtered ADC) in the watch window and type the numbers
//  into the matching table below. Keep ADC values in DESCENDING order
//  (close = big ADC value). Any number of rows up to IR_MAX_POINTS.
//
// OPTIONAL CURVE FIT (online curve-fitting tools are allowed in Lab5):
//  Fit  d = A/(n + B) + C  to your (n, d) points, put A,B,C below and set
//  IR_USE_FIT to 1. Same form as the Lab1 Convert routine.
// ---------------------------------------------------------------------------

#define IR_USE_FIT     0       // 0 = table interpolation, 1 = A/(n+B)+C fit
#define IR_FAR_MM      800     // returned when nothing is in range
#define IR_MAX_POINTS  12

typedef struct { int32_t adc; int32_t mm; } IRPoint;

// ----- calibration tables (ADC descending, mm ascending) -----
// Defaults are your Lab4 measurements (same numbers for all three sensors).
static const IRPoint IRLeftTable[] = {
  {16251, 50}, {14380,100}, {7269,200}, {5379,300}, {4331,400},
  { 3379,500}, { 3008,600}, {2842,700}, {2716,800}
};
static const IRPoint IRCenterTable[] = {
  {16251, 50}, {14380,100}, {7269,200}, {5379,300}, {4331,400},
  { 3379,500}, { 3008,600}, {2842,700}, {2716,800}
};
static const IRPoint IRRightTable[] = {
  {16251, 50}, {14380,100}, {7269,200}, {5379,300}, {4331,400},
  { 3379,500}, { 3008,600}, {2842,700}, {2716,800}
};
#define NPTS(t) ((int)(sizeof(t)/sizeof((t)[0])))

#if IR_USE_FIT
// ----- curve-fit constants: d(mm) = A/(n+B) + C -----
static const int32_t AL = 1195172, BL = -1058, CL = 0;   // placeholders - refit!
static const int32_t AC = 1195172, BC = -1058, CC = 0;
static const int32_t AR = 1195172, BR = -1058, CR = 0;
#endif

// Piecewise-linear interpolation through a calibration table.
static int32_t IR_TableConvert(int32_t n, const IRPoint *t, int npts){
  int i;
  if(n >= t[0].adc) return t[0].mm;             // closer than first point
  if(n <= t[npts-1].adc) return IR_FAR_MM;      // farther than last point
  for(i = 0; i < npts-1; i++){
    if(n <= t[i].adc && n > t[i+1].adc){
      // linear between (t[i].adc, t[i].mm) and (t[i+1].adc, t[i+1].mm)
      return t[i].mm + ((t[i].adc - n)*(t[i+1].mm - t[i].mm))
                       /(t[i].adc - t[i+1].adc);
    }
  }
  return IR_FAR_MM;
}

#if IR_USE_FIT
static int32_t IR_FitConvert(int32_t n, int32_t A, int32_t B, int32_t C){
  int32_t d;
  if(n + B <= 0) return IR_FAR_MM;
  d = A/(n + B) + C;
  if(d > IR_FAR_MM || d < 0) d = IR_FAR_MM;
  return d;
}
#endif

int32_t LeftConvert(int32_t nl){        // returns left distance in mm
#if IR_USE_FIT
  return IR_FitConvert(nl, AL, BL, CL);
#else
  return IR_TableConvert(nl, IRLeftTable, NPTS(IRLeftTable));
#endif
}

int32_t CenterConvert(int32_t nc){      // returns center distance in mm
#if IR_USE_FIT
  return IR_FitConvert(nc, AC, BC, CC);
#else
  return IR_TableConvert(nc, IRCenterTable, NPTS(IRCenterTable));
#endif
}

int32_t RightConvert(int32_t nr){       // returns right distance in mm
#if IR_USE_FIT
  return IR_FitConvert(nr, AR, BR, CR);
#else
  return IR_TableConvert(nr, IRRightTable, NPTS(IRRightTable));
#endif
}
