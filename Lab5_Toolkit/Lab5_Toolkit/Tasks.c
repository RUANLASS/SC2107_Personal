// Tasks.c - Lab5 practice M/H tasks built on Toolkit.c
//
// Every task has a header block with:
//   USES    sensors / actuators / LEDs
//   METHOD  polling or interrupt
//   CALLS   the toolkit primitives it relies on
//   PAUSE   where the SW1/SW2 start happens (main.c calls WaitForStart()
//           before every task unless the task says it waits for its own button)
//   TUNE    #defines you are most likely to change on the day
//   ASK     ambiguities to clarify with the instructor before you start
//
// All "forever" tasks end when you press SW1 or SW2 (StopRequested()), and
// main.c then waits for another press, so you can retry without the USB.

#include <stdint.h>
#include "msp.h"
#include "Toolkit.h"
#include "Tasks.h"

#define SPIN_SPEED 1800   // in-place search spin

// ====================== shared line-following helper ======================
#define CORNER_POS 230      // |position| beyond this = line is at the outer sensors -> sharp turn now
static uint32_t LastSeen;
static int32_t  LastPos;
static int8_t   LastSide;   // +1 = line last went RIGHT, -1 = LEFT
static void Follow_Reset(void){ LastSeen = Millis; LastPos = 0; LastSide = 1; }

// Pivot toward 'side' until the two centre sensors see the line (max ms).
// Returns 1 if centred on the line, 0 if it timed out.
static uint8_t PivotToLine(int8_t side, uint32_t ms){
  uint32_t t0 = Millis;
  uint8_t d;
  do {
    if(side >= 0) Drive(SPIN_SPEED, -SPIN_SPEED);
    else          Drive(-SPIN_SPEED, SPIN_SPEED);
    d = Line_Read();
  } while(!(d & REF_CENTER2) && (Millis - t0 < ms));
  Stop();
  return (d & REF_CENTER2) != 0;
}

// One step of line following, with sharp-corner handling.
// 1. Line at the OUTER sensors (|pos| > CORNER_POS): a sharp corner is
//    starting -> stop and pivot that way now, before the robot runs past it.
// 2. Line lost for > lostMs (already ran past the corner): back up slowly
//    until any sensor sees the tape again (undo the coasting), then pivot
//    toward the side the line last went.
// 3. Otherwise normal P-control steering.
// Returns sensor data (0 = no line right now).
static uint8_t Follow_Step(uint16_t base, uint32_t lostMs){
  uint8_t data = Line_Read();
  int32_t corr;
  uint32_t t0;
  if(data){
    LastSeen = Millis;
    LastPos = Line_Position(data);           // + = line on the right
    if(LastPos >  140) LastSide =  1;
    if(LastPos < -140) LastSide = -1;
    if(LastPos > CORNER_POS || LastPos < -CORNER_POS){
      Stop();
      PivotToLine(LastSide, 3000);           // sharp corner: turn on the spot
      LastSeen = Millis;
      return data;
    }
    corr = (LastPos * LF_KP_NUM) / LF_KP_DEN;
    Drive((int32_t)base + corr, (int32_t)base - corr);
    return data;
  }
  if(Millis - LastSeen > lostMs){            // ran past the line
    Stop();
    t0 = Millis;
    Drive(-SPEED_SLOW, -SPEED_SLOW);         // back up onto the tape
    while(!Line_Read() && (Millis - t0 < 800)){}
    Stop();
    if(!PivotToLine(LastSide, 4000)) LastSide = -LastSide;  // not found: other way next time
    LastSeen = Millis;
  }
  return 0;
}

// Prints "label=value" helpers for the serial terminal
static void SerKV(const char *k, int32_t v){ Ser_Str(k); Ser_SDec(v); Ser_Str("  "); }

// =========================================================================
//                      PRE-KNOWN DEMO + CALIBRATION
// =========================================================================

// USES    reflectance, bumps, IR (all at the desk, USB connected)
// METHOD  polling, values in globals -> add PK_* and IR_mm to the WATCH window
//         (CCS: View > Expressions, enable continuous refresh)
// SHOW    Lab2: PK_Reflect bit n-1 = 1 when sensor n is over black tape
//         Lab3: PK_BumpNum = which bump is pressed (0..5), PK_Bump = pattern
//         Lab4: IR_mm[0..2] = L, C, R distance in mm (box at 200 mm -> ~200)
volatile uint8_t PK_Reflect, PK_Bump;
volatile int8_t  PK_BumpNum;
void Task_PreKnown(void){
  uint32_t tPrint = 0;
  while(1){
    PK_Reflect = Line_Read();
    PK_Bump    = Bump_Now();
    PK_BumpNum = Bump_LowestIndex(PK_Bump);
    IR_Read(0, 0, 0);
    LED_RGB(PK_Bump ? LED_BLUE : (PK_Reflect ? LED_GREEN : LED_OFF));
    if(Millis - tPrint > 500){           // also on the serial terminal
      tPrint = Millis;
      Ser_Str("Ref="); Ser_Bin(PK_Reflect, 8);
      Ser_Str("  Bump="); Ser_Bin(PK_Bump, 6);
      Ser_Str("  #"); Ser_SDec(PK_BumpNum);
      Ser_Str("  IR L/C/R mm="); Ser_SDec(IR_mm[0]); Ser_Char('/');
      Ser_SDec(IR_mm[1]); Ser_Char('/'); Ser_SDec(IR_mm[2]); Ser_NL();
    }
    Delay_ms(20);
  }
}

// Desk calibration: push the wheels by hand to see tach counts, hold a box
// at 10/15/20/30 cm to get IR raw values for the tables in IRDistance.c.
void Task_Calibrate(void){
  Tach_Reset();
  while(!StopRequested()){
    IR_Read(0, 0, 0);
    SerKV("rawL=", IR_Raw[0]); SerKV("rawC=", IR_Raw[1]); SerKV("rawR=", IR_Raw[2]);
    SerKV("mmL=", IR_mm[0]);   SerKV("mmC=", IR_mm[1]);   SerKV("mmR=", IR_mm[2]);
    Ser_Str("ref="); Ser_Bin(Line_Read(), 8); Ser_Str("  ");
    Ser_Str("bump="); Ser_Bin(Bump_Now(), 6); Ser_Str("  ");
    SerKV("tachL=", Tach_Left()); SerKV("tachR=", Tach_Right());
    SerKV("mm=", Tach_DistanceMM()); Ser_NL();
    Delay_ms(300);
  }
}

void Test_Square(void){
  uint8_t i;
  for(i = 0; i < 4; i++){
    Move_mm(300, SPEED_MED, 0); Delay_ms(300);
    Turn_deg(90, TURN_SPEED);   Delay_ms(300);
  }
}
void Test_Turn360(void){ Turn_deg(360, TURN_SPEED); }
void Test_Straight1m(void){ Move_mm(1000, SPEED_MED, 0); }
void Test_LineFollow(void){
  Follow_Reset();
  while(!StopRequested()){ Follow_Step(SPEED_SLOW, 50); Delay_ms(2); }
}

// =========================================================================
//                                 M-TASKS
// =========================================================================

// M1  Drive forward. When reflectance sensor #M1_SENSOR sees black, turn 90
//     deg left and continue.
// USES    reflectance, motors, tachometer (turn)
// METHOD  polling
// CALLS   Drive, Line_Read, Turn_deg(-90)
// PAUSE   main: WaitForStart() before
// ASK     continue forever? turn once only? which way is "left" if robot faces you?
#define M1_SENSOR 3
void Task_M1_LineTurnLeft(void){
  while(!StopRequested()){
    Drive(SPEED_MED, SPEED_MED);
    if(Line_Read() & REF_S(M1_SENSOR)){
      Stop(); Delay_ms(100);
      Turn_deg(-90, TURN_SPEED);
      Delay_ms(100);
      Drive(SPEED_SLOW, SPEED_SLOW);       // leave the tape before re-arming
      while(Line_Read() & REF_S(M1_SENSOR)){ if(StopRequested()) return; }
    }
    Delay_ms(2);
  }
}

// M2  Drive forward. When any bump is pressed: reverse 0.5 s, then turn 90
//     deg AWAY from the side that was hit.
// USES    bumps (interrupt flag + debounced read), motors
// METHOD  GPIO interrupt sets a flag, main loop reacts
// CALLS   Bump_TakeEvent, Bump_Stable, Move_ms, Turn_deg
// TUNE    which bumps count as "right" (BUMP_RIGHTSIDE = Bump0..2)
void Task_M2_BumpReverseTurn(void){
  uint8_t hit;
  Bump_TakeEvent();
  while(!StopRequested()){
    Drive(SPEED_MED, SPEED_MED);
    if(Bump_TakeEvent() || Bump_Now()){
      hit = Bump_Now();
      Stop();
      if(hit == 0) hit = Bump_Stable();
      Move_ms(-SPEED_MED, -SPEED_MED, 500);
      if(Count_Ones(hit & BUMP_RIGHTSIDE) >= Count_Ones(hit & BUMP_LEFTSIDE))
        Turn_deg(-90, TURN_SPEED);         // hit on the right -> turn left
      else
        Turn_deg(90, TURN_SPEED);          // hit on the left -> turn right
      Bump_TakeEvent();                    // ignore bounces from the manoeuvre
    }
  }
}

// M3  Drive forward. If centre IR < 15 cm: stop, reverse 10 cm, turn 90 right.
// USES    IR centre, motors, tachometer
// METHOD  polling (IR_Read ~0.5 ms)
// CALLS   IR_Center, Move_mm(-100), Turn_deg(90)
#define M3_STOP_MM 150
void Task_M3_IRStopReverseTurn(void){
  while(!StopRequested()){
    Drive(SPEED_MED, SPEED_MED);
    if(IR_Center() < M3_STOP_MM){
      Stop(); Delay_ms(200);
      Move_mm(-100, SPEED_MED, 0); Delay_ms(200);
      Turn_deg(90, TURN_SPEED);    Delay_ms(200);
    }
  }
}

// M4  Stay stationary until SW2, then line-follow for 5 s, then stop.
// USES    SW2, reflectance, motors, Millis
// METHOD  polling
// PAUSE   OWN: waits for SW2 itself (main does not call WaitForStart)
void Task_M4_SW2LineFollow5s(void){
  uint32_t t0;
  Stop();
  WaitForButton(SW2);
  Follow_Reset();
  t0 = Millis;
  while(Millis - t0 < 5000){ Follow_Step(SPEED_MED, 300); Delay_ms(2); }
  Stop();
  LED_Flash(LED_GREEN, 2, 200);
}

// M5  Treat Bump0..Bump2 as a 3-bit number, show 0..7 on the LEDs.
// USES    bumps, RGB LED, serial
// METHOD  polling with debounce
// TUNE    M5_BLINK 0: RGB colour = value (0 off,1 red,2 green,3 yellow,
//         4 blue,5 pink,6 sky,7 white). 1: blink the RGB white 'value' times.
// ASK     colour code or blink count? what should 0 look like?
#define M5_BLINK 0
void Task_M5_BumpBinaryLED(void){
  uint8_t v, last = 0xFF;
  while(!StopRequested()){
    v = Bump_Stable() & 0x07;
    if(v != last){
      last = v;
      Ser_Str("Bumps0-2="); Ser_Bin(v, 3); Ser_Str("  dec="); Ser_Dec(v); Ser_NL();
#if M5_BLINK
      LED_BlinkCount(v, LED_WHITE);
#endif
    }
#if !M5_BLINK
    LED_RGB(v);
#endif
  }
  LED_RGB(LED_OFF);
}

// M6  Red LED if any of the middle four sensors sees black; green LED if
//     only the outermost sensors see black.
// USES    reflectance, RGB LED
// METHOD  polling
// ASK     "red LED" = RGB red or LED1? what when both / neither?
void Task_M6_ReflectanceLED(void){
  uint8_t d;
  while(!StopRequested()){
    d = Line_Read();
    if(d & REF_CENTER4)    { LED_RGB(LED_RED);   LED_Red1(1); }
    else if(d & REF_OUTER) { LED_RGB(LED_GREEN); LED_Red1(0); }
    else                   { LED_RGB(LED_OFF);   LED_Red1(0); }
    Delay_ms(10);
  }
  LED_RGB(LED_OFF); LED_Red1(0);
}

// M7  Drive forward until the LEFT wheel has turned the counts for 30 cm.
// USES    tachometer (left wheel only), motors
// METHOD  polling of tachometer counts (counts come from TimerA3 capture ISR)
// CALLS   Tach_Reset, Tach_Left, MM_ToCounts
#define M7_MM 300
void Task_M7_TachDistance(void){
  int32_t target = (int32_t)MM_ToCounts(M7_MM) - STOP_EARLY_COUNTS;
  int32_t corr;
  Tach_Reset();
  while(Tach_Left() < target){
    corr = (Tach_Left() - Tach_Right()) * STRAIGHT_KP;   // keep it straight
    Drive(SPEED_MED - corr, SPEED_MED + corr);
    Delay_ms(2);
  }
  Stop();
  Ser_Str("left counts="); Ser_Dec(Tach_Left()); Ser_Str(" target="); Ser_Dec(target); Ser_NL();
}

// M8  Corridor: drive forward; if left IR is closer than right by > 5 cm,
//     steer right (and vice versa).
// USES    IR left/right (+centre as safety stop), motors
// METHOD  polling
// TUNE    M8_STEER (how hard to steer), M8_DEADBAND_MM
#define M8_DEADBAND_MM 50
#define M8_STEER       800
void Task_M8_Corridor(void){
  int32_t l, c, r;
  while(!StopRequested()){
    IR_Read(&l, &c, &r);
    if(c < 100 || Bump_Now()){ Stop(); continue; }        // safety
    if(l + M8_DEADBAND_MM < r)      Drive(SPEED_MED + M8_STEER, SPEED_MED - M8_STEER); // wall on left -> right
    else if(r + M8_DEADBAND_MM < l) Drive(SPEED_MED - M8_STEER, SPEED_MED + M8_STEER); // wall on right -> left
    else                            Drive(SPEED_MED, SPEED_MED);
    Delay_ms(5);
  }
}

// M9  Wait for SW1. Then count bump presses for 10 s and show the count.
// USES    SW1, bumps, LEDs (binary 0-15), serial, Millis
// METHOD  polling with debounce, counts 0 -> pressed transitions
// PAUSE   OWN: waits for SW1 itself
// ASK     display format if count > 15? (we print it on serial too)
void Task_M9_CountBumps10s(void){
  uint32_t t0, count = 0;
  uint8_t prev = 0, now;
  Stop();
  LED_ShowNumber(0);
  WaitForButton(SW1);
  t0 = Millis;
  while(Millis - t0 < 10000){
    now = Bump_Stable();
    if(now && !prev){ count++; LED_ShowNumber(count); }
    prev = now;
  }
  Ser_Str("Bump presses in 10 s = "); Ser_Dec(count); Ser_NL();
  LED_ShowNumber(count);                 // stays on until you press a button
  WaitForButton(SW_ANY);
  LED_ShowNumber(0);
}

// M10 When sensor #1 sees black, drive forward exactly 2 s, then stop.
// USES    reflectance #1, motors, Millis (1 ms tick -> accurate timing)
// METHOD  polling
// TUNE    M10_MODE 0: robot waits stationary until sensor 1 is over black
//                1: robot drives slowly until sensor 1 sees black, then 2 s
// ASK     which of the two the instructor means
#define M10_MODE 0
void Task_M10_LineThenDrive2s(void){
  uint32_t t0;
  if(M10_MODE) Drive(SPEED_SLOW, SPEED_SLOW);
  while(!(Line_Read() & REF_S(1))){ if(StopRequested()) return; }
  t0 = Millis;
  Drive(SPEED_MED, SPEED_MED);
  while(Millis - t0 < 2000){}
  Stop();
}

// =========================================================================
//                                 H-TASKS
// =========================================================================

// H1  Bump0..5 = bit0..5. On every change, show the 6-bit value in binary,
//     decimal and hex on the serial terminal. No conversion library.
// USES    bumps (GPIO interrupt on press AND release), UART0, RGB
// METHOD  PORT4 interrupt -> flag -> debounce in main -> convert with
//         Num_ToDec/Num_ToHex (our own digit loops, no sprintf/UART0_OutUDec)
// PAUSE   none needed (runs at the desk) - main still waits for SW1/SW2
// TERMINAL 115200 8N1 (TExaSdisplay / PuTTY / CCS terminal)
void Task_H1_BumpBinDecHex(void){
  uint8_t v, last = 0xFF;
  char dec[4], hex[3], bin[7];
  Bump_BothEdges(1);
  Bump_TakeEvent();
  Ser_Str("\r\nH1: press bump switches (Bump0 = bit0 ... Bump5 = bit5)\r\n");
  while(!StopRequested()){
    if(Bump_TakeEvent() || last == 0xFF){
      v = Bump_Stable();
      if(v != last){
        last = v;
        Num_ToBin(v, bin, 6);
        Num_ToDec(v, dec);
        Num_ToHex(v, hex, 2);
        Ser_Str("Binary: "); Ser_Str(bin);
        Ser_Str("   Decimal: "); Ser_Str(dec);
        Ser_Str("   Hex: 0x"); Ser_Str(hex); Ser_NL();
        LED_RGB(v & 0x07);
      }
    }
  }
  Bump_BothEdges(0);
  LED_RGB(LED_OFF);
}

// H2  Follow a black line. If the line is lost > 1 s, spin in place until it
//     is found, then resume. Park (stop) on the end pattern (all 8 black).
// USES    reflectance, motors, Millis
// METHOD  polling, P-controller on Reflectance_Position
// CALLS   Follow_Step (Line_FollowStep + lost-line spin)
// TUNE    H2_END_SENSORS (8 = all black; 7 is more forgiving), H2_LOST_MS
#define H2_LOST_MS     1000
#define H2_END_SENSORS 8
#define H2_END_READS   3      // consecutive readings to confirm the end bar
void Task_H2_LineRecoveryPark(void){
  uint8_t d, endCount = 0;
  Follow_Reset();
  while(!StopRequested()){
    d = Follow_Step(SPEED_MED, H2_LOST_MS);
    LED_RGB(d ? LED_GREEN : LED_RED);
    if(Count_Ones(d) >= H2_END_SENSORS){
      if(++endCount >= H2_END_READS){
        Stop();
        LED_Flash(LED_WHITE, 3, 200);
        return;
      }
    } else endCount = 0;
    Delay_ms(2);
  }
  LED_RGB(LED_OFF);
}

// H3  Drive forward. If any IR < 20 cm: reverse, turn 90 away, continue.
//     Count obstacles avoided and display on LEDs + serial.
// USES    IR L/C/R, motors, tachometer, LEDs, UART
// METHOD  polling
// TUNE    H3_FRONT_MM, H3_SIDE_MM (side sensors may need a smaller threshold)
#define H3_FRONT_MM 200
#define H3_SIDE_MM  200
void Task_H3_AvoidAndCount(void){
  int32_t l, c, r;
  uint32_t count = 0;
  LED_ShowNumber(0);
  while(!StopRequested()){
    Drive(SPEED_MED, SPEED_MED);
    IR_Read(&l, &c, &r);
    if(c < H3_FRONT_MM || l < H3_SIDE_MM || r < H3_SIDE_MM){
      Stop(); Delay_ms(100);
      Move_mm(-100, SPEED_MED, 0);
      if(l < r) Turn_deg(90, TURN_SPEED);   // obstacle more on the left -> turn right
      else      Turn_deg(-90, TURN_SPEED);  // more on the right (or equal) -> turn left
      count++;
      LED_ShowNumber(count);
      Ser_Str("Obstacles avoided: "); Ser_Dec(count);
      Ser_Str("  (L/C/R mm "); Ser_SDec(l); Ser_Char('/'); Ser_SDec(c);
      Ser_Char('/'); Ser_SDec(r); Ser_Str(")"); Ser_NL();
      Delay_ms(200);
    }
  }
}

// H4  On SW2: precise 180 deg turn with tachometer feedback, then 40 cm forward.
// USES    SW2, tachometer, motors
// METHOD  polling of tach counts
// PAUSE   OWN: waits for SW2 itself
// TUNE    TURN_CAL_PCT / DIST_CAL_PCT in Toolkit.h (use Test_Turn360 first!)
void Task_H4_Turn180Drive40(void){
  Stop();
  WaitForButton(SW2);
  Delay_ms(500);
  Turn_deg(180, TURN_SPEED);
  Delay_ms(300);
  Move_mm(400, SPEED_MED, 0);
  Ser_Str("Done. Tach mm = "); Ser_SDec(Tach_DistanceMM()); Ser_NL();
}

// H5  4-state machine: SEARCH (spin for line) -> FOLLOW (line following)
//     -> AVOID (IR < 15 cm) -> STOP (two bumps on one side pressed).
// USES    reflectance, IR centre, bumps, motors, RGB shows the state
// METHOD  polling FSM, one state update per loop
// TUNE    H5_AVOID_MM, H5_STOP_BUMPS (how many bumps on one side = STOP)
// ASK     what AVOID should do exactly, and whether STOP is final
typedef enum {SEARCH, FOLLOW, AVOID, STOPPED_ST} H5State_t;
#define H5_AVOID_MM   150
#define H5_STOP_BUMPS 2
static const char *H5Name[] = {"SEARCH", "FOLLOW", "AVOID", "STOP"};
static const uint8_t H5Color[] = {LED_BLUE, LED_GREEN, LED_RED, LED_WHITE};
void Task_H5_FSM(void){
  H5State_t state = SEARCH, prev = STOPPED_ST;
  uint8_t b, d;
  Follow_Reset();
  while(!StopRequested()){
    // highest priority transitions first
    b = Bump_Now();
    if(Count_Ones(b & BUMP_RIGHTSIDE) >= H5_STOP_BUMPS ||
       Count_Ones(b & BUMP_LEFTSIDE)  >= H5_STOP_BUMPS){
      state = STOPPED_ST;
    } else if((state == SEARCH || state == FOLLOW) && IR_Center() < H5_AVOID_MM){
      state = AVOID;
    }
    if(state != prev){                   // entry action: report + colour
      Ser_Str("State -> "); Ser_Str(H5Name[state]); Ser_NL();
      LED_RGB(H5Color[state]);
      prev = state;
    }
    switch(state){
      case SEARCH:
        Drive(SPIN_SPEED, -SPIN_SPEED);    // spin slowly
        if(Line_Read()){ Follow_Reset(); state = FOLLOW; }
        break;
      case FOLLOW:
        d = Line_FollowStep(SPEED_MED);
        if(d) LastSeen = Millis;
        else if(Millis - LastSeen > 300) state = SEARCH;
        break;
      case AVOID:
        Stop(); Delay_ms(100);
        Move_mm(-100, SPEED_MED, 0);
        Turn_deg(90, TURN_SPEED);
        Move_mm(150, SPEED_MED, Cond_Bump);
        state = SEARCH;
        break;
      case STOPPED_ST:
        Stop();
        break;
    }
    Delay_ms(2);
  }
  LED_RGB(LED_OFF);
}

// H6  Bump0..2 select speed (slow/med/fast), Bump3..5 select direction
//     (forward/left/right). Motor command follows the bump combination.
// USES    bumps, motors
// METHOD  polling with debounce
// TUNE    H6_LATCH 0: act only while held (literal reading of the task)
//                  1: remember the last speed/direction (tap to select)
// ASK     held vs latched, "left" = spin in place or curve?
#define H6_LATCH 0
void Task_H6_BumpSpeedDir(void){
  uint8_t b;
  int32_t speed = SPEED_MED;
  int8_t dir = 0;                         // 0 stop, 1 fwd, 2 left, 3 right
  while(!StopRequested()){
    b = Bump_Stable();
    if(b & BUMP(2))      speed = SPEED_FAST;   // highest pressed bit wins
    else if(b & BUMP(1)) speed = SPEED_MED;
    else if(b & BUMP(0)) speed = SPEED_SLOW;
    else if(!H6_LATCH)   speed = SPEED_MED;
    if(b & BUMP(3))      dir = 1;
    else if(b & BUMP(4)) dir = 2;
    else if(b & BUMP(5)) dir = 3;
    else if(!H6_LATCH)   dir = 0;
    switch(dir){
      case 1: Drive(speed, speed);  break;
      case 2: Drive(-speed, speed); break;   // spin left
      case 3: Drive(speed, -speed); break;   // spin right
      default: Stop();              break;
    }
    LED_RGB(dir == 0 ? LED_OFF : (speed == SPEED_FAST ? LED_RED :
            (speed == SPEED_MED ? LED_YELLOW : LED_GREEN)));
  }
  LED_RGB(LED_OFF);
}

// H7  Drive forward. When centre IR is 25 cm (+-3 cm): stop, flash all LEDs
//     3x, reverse 15 cm, turn 90 left, stop.
// USES    IR centre, motors, tachometer, LEDs
// METHOD  polling; slow approach so it stops inside the window; if it starts
//         too close it backs off into the window
// TUNE    H7_TARGET_MM / H7_TOL_MM, approach speed
#define H7_TARGET_MM 250
#define H7_TOL_MM    30
void Task_H7_DistanceSequence(void){
  int32_t c;
  uint8_t inBand = 0;
  while(inBand < 3){                     // 3 readings in a row inside the window
    if(StopRequested()) return;
    c = IR_Center();
    if(c > H7_TARGET_MM + H7_TOL_MM){ Drive(SPEED_SLOW, SPEED_SLOW);   inBand = 0; }
    else if(c < H7_TARGET_MM - H7_TOL_MM){ Drive(-SPEED_SLOW, -SPEED_SLOW); inBand = 0; }
    else { Stop(); inBand++; }
    Delay_ms(5);
  }
  Stop();
  Ser_Str("Stopped at centre IR = "); Ser_SDec(IR_Center()); Ser_Str(" mm"); Ser_NL();
  LED_Flash(LED_WHITE, 3, 250);          // RGB white + LED1 = "all LEDs"
  Move_mm(-150, SPEED_MED, 0);
  Delay_ms(200);
  Turn_deg(-90, TURN_SPEED);
}

// H8  Line following. If any bump: stop, back up 10 cm, turn 45 deg away from
//     the bumped side, resume line following.
// USES    reflectance, bumps (interrupt flag), motors, tachometer
// METHOD  polling FSM-ish loop + bump interrupt flag
// CALLS   Follow_Step (re-finds the line by spinning if it is lost > 500 ms)
void Task_H8_LineBumpOverride(void){
  uint8_t hit;
  Follow_Reset();
  Bump_TakeEvent();
  while(!StopRequested()){
    if(Bump_TakeEvent() || Bump_Now()){
      Stop();
      hit = Bump_Stable();
      Move_mm(-100, SPEED_MED, 0);
      if(Count_Ones(hit & BUMP_RIGHTSIDE) >= Count_Ones(hit & BUMP_LEFTSIDE))
        Turn_deg(-45, TURN_SPEED);       // bumped on the right -> turn left
      else
        Turn_deg(45, TURN_SPEED);
      Bump_TakeEvent();
      Follow_Reset();
    }
    Follow_Step(SPEED_MED, 500);
    Delay_ms(2);
  }
}

// H9  Start on a line, follow it to a wide black bar (all 8 black), turn 90,
//     drive into the parking bay marked by another black line, stop centred.
// USES    reflectance, motors, tachometer
// METHOD  polling, sequence of phases
// TUNE    H9_TURN_DEG (+90 right / -90 left), H9_MAX_MM (give up distance),
//         H9_EXTRA_MM (drive past the bay line so the robot BODY is centred:
//         the sensor bar is ahead of the wheels - measure on your robot)
// ASK     which way to turn, where exactly "centred" is
#define H9_TURN_DEG  90
#define H9_MAX_MM    600
#define H9_EXTRA_MM  0
void Task_H9_Parking(void){
  uint8_t d, endCount = 0;
  // Phase 1: follow the line to the wide bar
  Follow_Reset();
  while(1){
    if(StopRequested()) return;
    d = Follow_Step(SPEED_MED, 500);
    if(d == REF_ALL){ if(++endCount >= 3) break; } else endCount = 0;
    Delay_ms(2);
  }
  Stop(); Delay_ms(300);
  // Phase 2: turn into the bay
  Turn_deg(H9_TURN_DEG, TURN_SPEED); Delay_ms(300);
  // Phase 3: leave the bar, then drive until the bay line is under the sensors
  Move_mm(H9_MAX_MM, SPEED_SLOW, Cond_AllWhite);   // stops once off the black
  Move_mm(H9_MAX_MM, SPEED_SLOW, Cond_AnyBlack);   // stops on the bay line
  if(H9_EXTRA_MM) Move_mm(H9_EXTRA_MM, SPEED_SLOW, 0);
  Stop();
  LED_Flash(LED_GREEN, 3, 200);
}

// H10 Use the six bumps to enter a number 0..63 (each press TOGGLES its bit).
//     After SW1, drive forward that many cm. SW2 clears the number.
// USES    bumps, SW1/SW2, tachometer, motors, serial, RGB (low 3 bits)
// METHOD  polling with debounce; new presses = now & ~previous
// ASK     toggle-per-press vs hold-all-then-SW1 entry; cm or other units
void Task_H10_NumberEntryDrive(void){
  uint8_t value = 0, prev = 0, now, newly, sw;
  Ser_Str("\r\nH10: tap bumps to toggle bits, SW2 = clear, SW1 = go\r\n");
  while(1){
    now = Bump_Stable();
    newly = now & ~prev;
    prev = now;
    if(newly){
      value ^= newly;
      Ser_Str("Value: "); Ser_Bin(value, 6); Ser_Str(" = "); Ser_Dec(value); Ser_Str(" cm"); Ser_NL();
    }
    LED_RGB(value & 0x07);
    sw = Button_Read();
    if(sw){
      while(Button_Read()){}             // wait for release
      Delay_ms(20);
      if(sw & SW1) break;
      value = 0;                         // SW2 clears
      Ser_Str("Cleared"); Ser_NL();
    }
  }
  LED_RGB(LED_OFF);
  Delay_ms(START_DELAY_MS);              // hand clear of the robot
  Move_mm((int32_t)value * 10, SPEED_MED, 0);
  Ser_Str("Drove "); Ser_SDec(Tach_DistanceMM()); Ser_Str(" mm"); Ser_NL();
}



// =========================================================================
//                                 PRACTICE H-TASKS 27/10
// =========================================================================
void Task401_ObstacleCount(){}
void Task402_WallFollow(){}
void Task403_RotateScan(){}
void Task404_StateMach(){}
void Task405_BumpMode(){}
void Task406_Boundary(){}
void Task407_Password(){}
void Task408_LED_UDC(){}
void Task409_SumDiff(){}
void Task410_NumDrive(){}
