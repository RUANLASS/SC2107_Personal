// Toolkit.h  -  SC2107 Lab5 toolkit (RSLK / MSP432P401R)
// One header for everything a Lab5 L/M/H task usually needs.
// Built on the course drivers in ../inc (Motor, Reflectance, BumpInt,
// Tachometer, ADC14, IRDistance, LaunchPad, UART0, TimerA2, Clock).
//
// Resource map (so you know what is still FREE for a task):
//   TimerA0  PWM for motors            (Motor.c / PWM.c)
//   TimerA2  1 ms system tick          (Toolkit.c)    -> Millis
//   TimerA3  tachometer capture        (Tachometer.c)
//   PORT4    bump switch interrupt     (BumpInt.c)
//   ADC14    IR sensors, polled        (Toolkit.c)
//   UART0    serial terminal 115200    (UART0.c)
//   FREE:    TimerA1, SysTick, Timer32
//
// Conventions used everywhere in this toolkit:
//   Reflectance: bit = 1 means BLACK. Sensor #1 = bit0 = P7.0 = robot's RIGHT,
//                sensor #8 = bit7 = robot's LEFT.  REF_S(n) gives the mask.
//   Bumps:       POSITIVE logic, bit = 1 means PRESSED.
//                Bump0 = bit0 = robot's RIGHT ... Bump5 = bit5 = robot's LEFT.
//   Buttons:     SW1 = 0x01 (P1.1, left button), SW2 = 0x02 (P1.4, right button)
//   Motors:      signed duty, + = wheel forward, - = wheel backward,
//                range -MOTOR_MAX..+MOTOR_MAX (PWM period is 7500).
//   Turns:       + degrees = clockwise (RIGHT), - degrees = LEFT.
//   Distances:   mm.  IR returns IR_FAR (800) when nothing is in range.

#ifndef TOOLKIT_H_
#define TOOLKIT_H_
#include <stdint.h>

// =========================== CALIBRATION ===================================
// Tune these on YOUR robot before Lab5 (TASK 901-903 in main help you).
#define MOTOR_MAX          7000   // PWM period is 7500; keep below it
#define SPEED_SLOW         1500
#define SPEED_MED          2500
#define SPEED_FAST         4000
#define TURN_SPEED         1500   // duty used for tachometer turns

#define WHEEL_DIAMETER_MM  70     // TI-RSLK wheel
#define TRACK_WIDTH_MM     140    // distance between the two wheels
#define COUNTS_PER_REV     360    // tachometer steps per wheel revolution
#define DIST_CAL_PCT       98    // >100 if Move_mm() stops short
#define TURN_CAL_PCT       103    // >100 if Turn_deg() turns too little
#define STOP_EARLY_COUNTS  3      // stop a little early to allow for coasting
#define STRAIGHT_KP        40     // duty change per count of L/R mismatch

// Open-loop fallbacks (only if the tachometer misbehaves on the day)
#define MS_PER_90DEG       450    // at TURN_SPEED, measure with a stopwatch

// Line following
#define REF_TIME_US        1000   // Reflectance_Read() wait time
#define LF_KP_NUM          3      // steering = position * NUM / DEN
#define LF_KP_DEN          1      // (position is -332..+332)

// Start-up delay after SW1/SW2 so your hand is clear of the robot
#define START_DELAY_MS     1000

// ============================== CONSTANTS ==================================
// RGB LED colours (LaunchPad_Output)
#define LED_OFF     0x00
#define LED_RED     0x01
#define LED_GREEN   0x02
#define LED_YELLOW  0x03
#define LED_BLUE    0x04
#define LED_PINK    0x05
#define LED_SKYBLUE 0x06
#define LED_WHITE   0x07

#define SW1  0x01
#define SW2  0x02
#define SW_ANY 0x03

#define REF_S(n)      (1u << ((n)-1))   // reflectance sensor n = 1..8
#define REF_ALL       0xFF
#define REF_CENTER2   0x18              // sensors 4,5
#define REF_CENTER4   0x3C              // sensors 3..6
#define REF_OUTER     0x81              // sensors 1 and 8
#define REF_RIGHTHALF 0x0F              // sensors 1..4
#define REF_LEFTHALF  0xF0              // sensors 5..8
#define LINE_NONE     2147483647        // Reflectance_Position() when no line

#define BUMP(n)        (1u << (n))      // Bump n = 0..5
#define BUMP_RIGHTSIDE 0x07             // Bump0..2
#define BUMP_LEFTSIDE  0x38             // Bump3..5

#define IR_FAR  800

// ============================== CORE =======================================
void Toolkit_Init(void);              // call first; enables interrupts
extern volatile uint32_t Millis;      // ms since Toolkit_Init (TimerA2)
void Delay_ms(uint32_t ms);           // accurate, uses Millis

// ============================ BUTTONS / PAUSE ==============================
uint8_t Button_Read(void);            // 0, SW1, SW2 or SW1|SW2 (live)
uint8_t WaitForButton(uint8_t mask);  // blink LED1, wait press+release, return which
uint8_t WaitForStart(void);           // Stop, WaitForButton(SW1|SW2), START_DELAY_MS
uint8_t    TimedPause(uint32_t time);    // course version: run 'time' ms, stop, wait SW
uint8_t StopRequested(void);          // 1 if SW1/SW2 pressed (waits for release)

// ================================ LEDs =====================================
void LED_RGB(uint8_t color);          // LED_RED, LED_GREEN, ... (P2.0-2.2)
void LED_Red1(uint8_t on);            // the single red LED1 (P1.0)
void LED_Flash(uint8_t color, uint8_t times, uint32_t ms);
void LED_ShowNumber(uint32_t n);      // 0-15 in binary: RGB = bits0-2, LED1 = bit3
void LED_BlinkCount(uint32_t n, uint8_t color); // blink n times (n=0: one long blink)

// ============================ REFLECTANCE ==================================
uint8_t Line_Read(void);              // 8-bit, 1 = black (sensor1 = bit0)
int32_t Line_Position(uint8_t data);  // -332 (line on left) .. +332 (line on right)
uint8_t Line_FollowStep(uint16_t base); // one P-control step; returns data (0 = lost)
uint8_t Count_Ones(uint32_t v);       // number of 1 bits

// ================================ BUMP =====================================
uint8_t Bump_Now(void);               // positive logic, not debounced
uint8_t Bump_Stable(void);            // debounced (same value twice, 15 ms apart)
uint8_t Bump_TakeEvent(void);         // 1 if the bump ISR fired since last call
int8_t  Bump_LowestIndex(uint8_t b);  // 0..5 of lowest pressed bump, -1 if none
void    Bump_BothEdges(uint8_t on);   // 1: ISR fires on press AND release

// =============================== MOTORS ====================================
void Drive(int32_t left, int32_t right); // signed duty per wheel
void Stop(void);
void Move_ms(int32_t left, int32_t right, uint32_t ms); // drive for a time, then stop
void Turn_ms(int32_t deg);            // open-loop turn using MS_PER_90DEG

// ============================ TACHOMETER ===================================
void     Tach_Reset(void);            // zero the distance counters
int32_t  Tach_Left(void);             // |counts| since Tach_Reset
int32_t  Tach_Right(void);
int32_t  Tach_DistanceMM(void);       // average wheel distance since reset
uint32_t MM_ToCounts(uint32_t mm);
extern volatile int32_t SpeedL_mms, SpeedR_mms;  // wheel speed, updated every 100 ms

// Move / turn with tachometer feedback.
// cond: optional "stop early" test, e.g. Cond_Bump; pass 0 for none.
// Move returns 1 if cond stopped it, 2 if the wheels stalled, 0 if completed.
typedef uint8_t (*Cond_t)(void);
uint8_t Move_mm(int32_t mm, uint16_t duty, Cond_t cond); // + forward, - backward
void    Turn_deg(int32_t deg, uint16_t duty);           // + right, - left (pivot)
#define WHEEL_LEFT   0
#define WHEEL_RIGHT  1
void    Wheel_Turn_deg(uint8_t wheel, int32_t deg, uint16_t duty); // spin ONE wheel; + forward, - backward
void    Drive_AtSpeed(int32_t mm_per_s, uint32_t ms);   // closed-loop speed

// Ready-made stop conditions for Move_mm
uint8_t Cond_Bump(void);              // any bump pressed
uint8_t Cond_AnyBlack(void);          // any reflectance sensor black
uint8_t Cond_AllWhite(void);          // no reflectance sensor black
uint8_t Cond_IRClose(void);           // any IR < IR_CloseMM
extern int32_t IR_CloseMM;            // threshold for Cond_IRClose (default 200)

// ================================= IR ======================================
void    IR_Read(int32_t *left, int32_t *center, int32_t *right); // mm (averaged)
int32_t IR_Left(void);
int32_t IR_Center(void);
int32_t IR_Right(void);
extern volatile uint32_t IR_Raw[3];   // [0]=L [1]=C [2]=R raw ADC (watch window)
extern volatile int32_t  IR_mm[3];    // last distances in mm (watch window)

// ======================= NUMBER CONVERSION (no library) ====================
// All write a NUL-terminated string into buf and return the length.
uint8_t  Num_ToDec(uint32_t n, char *buf);               // 43   -> "43"
uint8_t  Num_ToSignedDec(int32_t n, char *buf);          // -5   -> "-5"
uint8_t  Num_ToHex(uint32_t n, char *buf, uint8_t digits); // 43,2 -> "2B"
uint8_t  Num_ToBin(uint32_t n, char *buf, uint8_t bits); // 43,6 -> "101011"
int32_t  Num_SignExtend(uint32_t v, uint8_t bits);       // 0x3F,6 -> -1
uint32_t Num_ToBCD(uint32_t n);                          // 43 -> 0x43
uint32_t Num_FromBCD(uint32_t bcd);                      // 0x43 -> 43
uint32_t Num_ReverseBits(uint32_t v, uint8_t bits);      // 0b001,3 -> 0b100
uint32_t Num_ParseDec(const char *s);                    // "123" -> 123
uint32_t Num_ParseHex(const char *s);                    // "2B" or "0x2B" -> 43

// ======================== SERIAL OUTPUT (UART0) ============================
void Ser_Str(const char *s);
void Ser_Char(char c);
void Ser_Dec(uint32_t n);
void Ser_SDec(int32_t n);
void Ser_Hex(uint32_t n, uint8_t digits);  // prints "0x" + digits
void Ser_Bin(uint32_t n, uint8_t bits);    // prints "0b" + bits
void Ser_NL(void);

#endif
