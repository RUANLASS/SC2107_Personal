// Lab5_Toolkitmain.c  -  SC2107 Lab5 task selector
//
// 1. Set TASK below, build, flash (Debug), press Resume/Run.
// 2. Unplug USB, carry the robot to the arena, place it.
// 3. LED1 blinks = waiting. Press SW1 or SW2 to start (1 s delay, LED1 solid).
// 4. Press SW1/SW2 during a "forever" task to stop it. LED1 blinks again,
//    press again to retry - no need to re-flash.
//
// TASK numbers
//   0   Pre-known demo (Lab2 reflectance, Lab3 bumps, Lab4 IR) - watch window
//   900 Desk calibration stream on serial (IR raw/mm, reflectance, bumps, tach)
//   901 Square test   902 Turn 360 test   903 1 m straight   904 line follow
//   101..110  M1..M10        201..210  H1..H10   (see Tasks.c)
//
// For a NEW task on the day: copy the closest Task_* in Tasks.c, rename it,
// add a case below. Keep the header notes - they double as your plan.

#include <stdint.h>
#include "msp.h"
#include "Toolkit.h"
#include "Tasks.h"

#define TASK 409

typedef struct {
  uint16_t id;
  void (*fn)(void);
  uint8_t ownStart;   // 1 = task waits for its own button, skip WaitForStart
} TaskEntry;

static const TaskEntry Table[] = {
  {  0, Task_PreKnown,              1},
  {900, Task_Calibrate,             1},
  {901, Test_Square,                0},
  {902, Test_Turn360,               0},
  {903, Test_Straight1m,            0},
  {904, Test_LineFollow,            0},
  {101, Task_M1_LineTurnLeft,       0},
  {102, Task_M2_BumpReverseTurn,    0},
  {103, Task_M3_IRStopReverseTurn,  0},
  {104, Task_M4_SW2LineFollow5s,    1},
  {105, Task_M5_BumpBinaryLED,      0},
  {106, Task_M6_ReflectanceLED,     0},
  {107, Task_M7_TachDistance,       0},
  {108, Task_M8_Corridor,           0},
  {109, Task_M9_CountBumps10s,      1},
  {110, Task_M10_LineThenDrive2s,   0},
  {201, Task_H1_BumpBinDecHex,      0},
  {202, Task_H2_LineRecoveryPark,   0},
  {203, Task_H3_AvoidAndCount,      0},
  {204, Task_H4_Turn180Drive40,     1},
  {205, Task_H5_FSM,                0},
  {206, Task_H6_BumpSpeedDir,       0},
  {207, Task_H7_DistanceSequence,   0},
  {208, Task_H8_LineBumpOverride,   0},
  {209, Task_H9_Parking,            0},
  {210, Task_H10_NumberEntryDrive,  0},
//  {401, Task401_ObstacleCount, 0},
  {402, Task_WallFollow, 0},
  {403, Task403_RotateScan, 0},
//  {404, Task404_StateMach, 0},
//  {405, Task405_BumpMode, 0},
  {406, Task406_Boundary, 0},
//  {407, Task407_Password, 0},
//  {408, Task408_LED_UDC, 0},
  {409, Task409_SumDiff, 0},
//  {410, Task410_NumDrive, 0}
};


int main(void){
  const TaskEntry *t = 0;
  uint16_t i;
  Toolkit_Init();
  for(i = 0; i < sizeof(Table)/sizeof(Table[0]); i++){
    if(Table[i].id == TASK) t = &Table[i];
  }
  if(t == 0){                       // unknown TASK number: fast red blink
    while(1){ LED_Flash(LED_RED, 1, 100); }
  }
  Ser_Str("\r\nLab5 toolkit, TASK "); Ser_Dec(TASK); Ser_NL();
  while(1){
    if(!t->ownStart) WaitForStart();
    t->fn();
    Stop();
    LED_RGB(LED_OFF);
    LED_Red1(0);
    if(t->ownStart) WaitForButton(SW_ANY);   // pause before re-running
  }
}
