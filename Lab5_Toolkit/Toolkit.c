// Toolkit.c  -  SC2107 Lab5 toolkit implementation. See Toolkit.h.
#include <stdint.h>
#include "msp.h"
#include "Toolkit.h"
#include "../inc/Clock.h"
#include "../inc/CortexM.h"
#include "../inc/LaunchPad.h"
#include "../inc/Motor.h"
#include "../inc/Reflectance.h"
#include "../inc/BumpInt.h"
#include "../inc/Tachometer.h"
#include "../inc/ADC14.h"
#include "../inc/IRDistance.h"
#include "../inc/UART0.h"
#include "../inc/TimerA2.h"

// step counters kept by Tachometer.c
extern int Tachometer_LeftSteps, Tachometer_RightSteps;

volatile uint32_t Millis = 0;
volatile int32_t SpeedL_mms = 0, SpeedR_mms = 0;
static volatile uint8_t BumpEventFlag = 0;

// ---------------------------------------------------------------- tick ----
static void Tick_ISR(void){             // TimerA2, every 1 ms
  static int32_t lastL = 0, lastR = 0;
  static uint8_t n = 0;
  Millis++;
  if(++n >= 100){                       // every 100 ms: wheel speed
    int32_t l = Tachometer_LeftSteps, r = Tachometer_RightSteps;
    int32_t dl = l - lastL, dr = r - lastR;
    if(dl < 0) dl = -dl;
    if(dr < 0) dr = -dr;
    // counts/100ms -> mm/s : counts * (pi*D/COUNTS_PER_REV) * 10
    SpeedL_mms = (dl * 31416L * WHEEL_DIAMETER_MM) / (1000L * COUNTS_PER_REV);
    SpeedR_mms = (dr * 31416L * WHEEL_DIAMETER_MM) / (1000L * COUNTS_PER_REV);
    lastL = l; lastR = r; n = 0;
  }
}

static void Bump_ISRTask(uint8_t bumps){ // PORT4 ISR callback (BumpInt.c)
  (void)bumps;
  BumpEventFlag = 1;                    // P4->IFG already cleared by BumpInt.c
}

void Toolkit_Init(void){
  DisableInterrupts();
  Clock_Init48MHz();          // 48 MHz CPU, 12 MHz SMCLK
  LaunchPad_Init();           // SW1, SW2, LED1, RGB LED
  Motor_Init();               // PWM on TimerA0
  Motor_Stop();
  Reflectance_Init();
  BumpInt_Init(&Bump_ISRTask);
  Tachometer_Init();          // TimerA3 capture
  ADC0_InitSWTriggerCh17_12_16();
  UART0_Init();               // 115200 baud
  TimerA2_Init(&Tick_ISR, 500);   // 12MHz/4/6 = 500 kHz -> 500 counts = 1 ms
  EnableInterrupts();
}

void Delay_ms(uint32_t ms){
  uint32_t t0 = Millis;
  while((Millis - t0) < ms){}
}

// ------------------------------------------------------------- buttons ----
uint8_t Button_Read(void){ return LaunchPad_Input(); }

uint8_t WaitForButton(uint8_t mask){
  uint8_t which;
  while((LaunchPad_Input() & mask) == 0){
    LED_Red1((Millis / 250) & 1);       // blink LED1 = "waiting for you"
  }
  which = LaunchPad_Input() & mask;
  Delay_ms(20);                         // debounce
  while(LaunchPad_Input() & mask){}     // wait for release
  Delay_ms(20);
  LED_Red1(0);
  return which;
}

uint8_t WaitForStart(void){
  uint8_t which;
  Stop();
  which = WaitForButton(SW1 | SW2);
  LED_Red1(1);                          // solid = about to start
  Delay_ms(START_DELAY_MS);
  LED_Red1(0);
  BumpEventFlag = 0;                    // forget bumps from carrying the robot
  return which;
}

void TimedPause(uint32_t time){         // same behaviour as the Lab3/4 version
  Clock_Delay1ms(time);
  Motor_Stop();
  while(LaunchPad_Input() == 0){}
  while(LaunchPad_Input()){}
}

uint8_t StopRequested(void){
  if(LaunchPad_Input()){
    Stop();
    Delay_ms(20);
    while(LaunchPad_Input()){}
    Delay_ms(20);
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------- LEDs ----
void LED_RGB(uint8_t color){ LaunchPad_Output(color & 0x07); }
void LED_Red1(uint8_t on){ LaunchPad_LED(on ? 1 : 0); }

void LED_Flash(uint8_t color, uint8_t times, uint32_t ms){
  uint8_t i;
  for(i = 0; i < times; i++){
    LED_RGB(color); LED_Red1(1); Delay_ms(ms);
    LED_RGB(LED_OFF); LED_Red1(0); Delay_ms(ms);
  }
}

void LED_ShowNumber(uint32_t n){
  LED_RGB(n & 0x07);
  LED_Red1((n >> 3) & 1);
}

void LED_BlinkCount(uint32_t n, uint8_t color){
  uint32_t i;
  if(n == 0){ LED_RGB(color); Delay_ms(1000); LED_RGB(LED_OFF); return; }
  for(i = 0; i < n; i++){
    LED_RGB(color);   Delay_ms(250);
    LED_RGB(LED_OFF); Delay_ms(250);
  }
}

// --------------------------------------------------------- reflectance ----
uint8_t Line_Read(void){ return Reflectance_Read(REF_TIME_US); }
int32_t Line_Position(uint8_t data){ return Reflectance_Position(data); }

uint8_t Line_FollowStep(uint16_t base){
  uint8_t data = Line_Read();
  int32_t pos, corr;
  if(data == 0) return 0;               // lost: caller decides, motors unchanged
  pos = Reflectance_Position(data);     // + = line is to the robot's right
  corr = (pos * LF_KP_NUM) / LF_KP_DEN;
  Drive((int32_t)base + corr, (int32_t)base - corr); // line right -> speed up left
  return data;
}

uint8_t Count_Ones(uint32_t v){
  uint8_t c = 0;
  while(v){ c += v & 1; v >>= 1; }
  return c;
}

// ---------------------------------------------------------------- bump ----
uint8_t Bump_Now(void){ return Bump_Read(); }

uint8_t Bump_Stable(void){
  uint8_t a, b;
  a = Bump_Read();
  while(1){
    Delay_ms(15);
    b = Bump_Read();
    if(a == b) return a;
    a = b;
  }
}

uint8_t Bump_TakeEvent(void){
  if(BumpEventFlag){ BumpEventFlag = 0; return 1; }
  return 0;
}

int8_t Bump_LowestIndex(uint8_t b){
  int8_t i;
  for(i = 0; i < 6; i++){ if(b & (1u << i)) return i; }
  return -1;
}

void Bump_BothEdges(uint8_t on){
  BumpInt_BothEdges = on ? 1 : 0;
  if(on){ // arm for the opposite of the current level right now
    P4->IES = (P4->IES & ~0xED) | (P4->IN & 0xED);
  } else {
    P4->IES |= 0xED;                    // falling edge = press only
  }
  P4->IFG &= ~0xED;
}

// -------------------------------------------------------------- motors ----
static uint16_t clampDuty(int32_t d){
  if(d < 0) d = -d;
  if(d > MOTOR_MAX) d = MOTOR_MAX;
  return (uint16_t)d;
}

void Drive(int32_t left, int32_t right){
  uint16_t l = clampDuty(left), r = clampDuty(right);
  if(left >= 0 && right >= 0)      Motor_Forward(l, r);
  else if(left < 0 && right < 0)   Motor_Backward(l, r);
  else if(left >= 0 && right < 0)  Motor_Right(l, r);   // left fwd, right back
  else                             Motor_Left(l, r);    // left back, right fwd
}

void Stop(void){ Motor_Stop(); }

void Move_ms(int32_t left, int32_t right, uint32_t ms){
  Drive(left, right);
  Delay_ms(ms);
  Stop();
}

void Turn_ms(int32_t deg){
  uint32_t ms = ((deg < 0 ? -deg : deg) * MS_PER_90DEG) / 90;
  if(deg >= 0) Drive(TURN_SPEED, -TURN_SPEED);
  else         Drive(-TURN_SPEED, TURN_SPEED);
  Delay_ms(ms);
  Stop();
}

// ---------------------------------------------------------- tachometer ----
static int32_t TachL0 = 0, TachR0 = 0;

void Tach_Reset(void){
  TachL0 = Tachometer_LeftSteps;
  TachR0 = Tachometer_RightSteps;
}
int32_t Tach_Left(void){
  int32_t d = Tachometer_LeftSteps - TachL0;
  return d < 0 ? -d : d;
}
int32_t Tach_Right(void){
  int32_t d = Tachometer_RightSteps - TachR0;
  return d < 0 ? -d : d;
}
int32_t Tach_DistanceMM(void){
  // counts * pi * D / COUNTS_PER_REV
  return ((Tach_Left() + Tach_Right()) * 31416L * WHEEL_DIAMETER_MM)
         / (2L * 10000L * COUNTS_PER_REV);
}
uint32_t MM_ToCounts(uint32_t mm){
  // counts = mm * COUNTS_PER_REV / (pi * D), with distance calibration
  return (uint32_t)(((uint64_t)mm * COUNTS_PER_REV * 10000UL * DIST_CAL_PCT)
                    / (31416UL * WHEEL_DIAMETER_MM * 100UL));
}

uint8_t Move_mm(int32_t mm, uint16_t duty, Cond_t cond){
  int32_t dir = (mm >= 0) ? 1 : -1;
  int32_t target = (int32_t)MM_ToCounts(mm >= 0 ? mm : -mm) - STOP_EARLY_COUNTS;
  int32_t dl, dr, corr, lastSum = -1;
  uint32_t lastChange = Millis;
  if(target <= 0) return 0;
  Tach_Reset();
  while(1){
    dl = Tach_Left(); dr = Tach_Right();
    if((dl + dr) / 2 >= target) break;
    if(cond && cond()){ Stop(); return 1; }
    if(dl + dr != lastSum){ lastSum = dl + dr; lastChange = Millis; }
    else if(Millis - lastChange > 1000){ Stop(); return 2; }  // stalled / blocked
    corr = (dl - dr) * STRAIGHT_KP;      // left ahead -> slow left, speed right
    if(corr >  (int32_t)duty/2) corr =  duty/2;
    if(corr < -(int32_t)duty/2) corr = -(int32_t)duty/2;
    Drive(dir * ((int32_t)duty - corr), dir * ((int32_t)duty + corr));
    Delay_ms(2);
  }
  Stop();
  return 0;
}

void Turn_deg(int32_t deg, uint16_t duty){
  // pivot turn: each wheel travels pi*TRACK*deg/360; one count = pi*D/360
  // => counts per wheel = TRACK * deg / D  (pi cancels)
  int32_t a = deg < 0 ? -deg : deg;
  int32_t target = (a * TRACK_WIDTH_MM * COUNTS_PER_REV * TURN_CAL_PCT)
                   / (360L * WHEEL_DIAMETER_MM * 100L) - STOP_EARLY_COUNTS;
  int32_t sgn = deg >= 0 ? 1 : -1;      // + = right: left wheel fwd, right back
  uint8_t doneL = 0, doneR = 0;
  uint32_t t0 = Millis;
  if(target <= 0) return;
  Tach_Reset();
  while(!(doneL && doneR)){
    if(Tach_Left()  >= target) doneL = 1;
    if(Tach_Right() >= target) doneR = 1;
    Drive(doneL ? 0 : sgn * (int32_t)duty, doneR ? 0 : -sgn * (int32_t)duty);
    if(Millis - t0 > 5000) break;       // safety timeout
    Delay_ms(1);
  }
  Stop();
}

void Drive_AtSpeed(int32_t mm_per_s, uint32_t ms){
  // simple integral controller per wheel, updated every 100 ms (speed tick)
  int32_t dutyL = mm_per_s * 10, dutyR = mm_per_s * 10;  // rough first guess
  int32_t dir = mm_per_s >= 0 ? 1 : -1;
  int32_t target = mm_per_s >= 0 ? mm_per_s : -mm_per_s;
  uint32_t t0 = Millis, tNext = Millis + 200;
  if(dutyL < 0){ dutyL = -dutyL; dutyR = -dutyR; }
  while(Millis - t0 < ms){
    Drive(dir * dutyL, dir * dutyR);
    if((int32_t)(Millis - tNext) >= 0){
      dutyL += (target - SpeedL_mms) * 3;
      dutyR += (target - SpeedR_mms) * 3;
      if(dutyL < 0) dutyL = 0;
      if(dutyR < 0) dutyR = 0;
      if(dutyL > MOTOR_MAX) dutyL = MOTOR_MAX;
      if(dutyR > MOTOR_MAX) dutyR = MOTOR_MAX;
      tNext = Millis + 100;
    }
  }
  Stop();
}

int32_t IR_CloseMM = 200;
uint8_t Cond_Bump(void){ return Bump_Read() != 0; }
uint8_t Cond_AnyBlack(void){ return Line_Read() != 0; }
uint8_t Cond_AllWhite(void){ return Line_Read() == 0; }
uint8_t Cond_IRClose(void){
  int32_t l, c, r;
  IR_Read(&l, &c, &r);
  return (l < IR_CloseMM) || (c < IR_CloseMM) || (r < IR_CloseMM);
}

// ------------------------------------------------------------------ IR ----
volatile uint32_t IR_Raw[3];
volatile int32_t  IR_mm[3];
#define IR_SAMPLES 32

void IR_Read(int32_t *left, int32_t *center, int32_t *right){
  uint32_t r17, r12, r16, sR = 0, sC = 0, sL = 0;
  uint8_t i;
  for(i = 0; i < IR_SAMPLES; i++){      // ~0.5 ms total
    ADC_In17_12_16(&r17, &r12, &r16);   // 17 = right P9.0, 12 = center P4.1, 16 = left P9.1
    sR += r17; sC += r12; sL += r16;
  }
  IR_Raw[0] = sL / IR_SAMPLES;
  IR_Raw[1] = sC / IR_SAMPLES;
  IR_Raw[2] = sR / IR_SAMPLES;
  IR_mm[0] = LeftConvert(IR_Raw[0]);
  IR_mm[1] = CenterConvert(IR_Raw[1]);
  IR_mm[2] = RightConvert(IR_Raw[2]);
  if(left)   *left   = IR_mm[0];
  if(center) *center = IR_mm[1];
  if(right)  *right  = IR_mm[2];
}
int32_t IR_Left(void)  { IR_Read(0, 0, 0); return IR_mm[0]; }
int32_t IR_Center(void){ IR_Read(0, 0, 0); return IR_mm[1]; }
int32_t IR_Right(void) { IR_Read(0, 0, 0); return IR_mm[2]; }

// ---------------------------------------------- number conversion -----
uint8_t Num_ToDec(uint32_t n, char *buf){
  char tmp[11];
  uint8_t i = 0, len = 0;
  do { tmp[i++] = '0' + (n % 10); n /= 10; } while(n);  // digits backwards
  while(i) buf[len++] = tmp[--i];                         // reverse
  buf[len] = 0;
  return len;
}

uint8_t Num_ToSignedDec(int32_t n, char *buf){
  if(n < 0){
    buf[0] = '-';
    return 1 + Num_ToDec((uint32_t)(-(n + 1)) + 1, buf + 1); // safe for INT_MIN
  }
  return Num_ToDec((uint32_t)n, buf);
}

uint8_t Num_ToHex(uint32_t n, char *buf, uint8_t digits){
  // digits = 0 -> as many as needed
  static const char hex[] = "0123456789ABCDEF";
  char tmp[8];
  uint8_t i = 0, len = 0;
  do { tmp[i++] = hex[n & 0xF]; n >>= 4; } while(n && i < 8);
  while(i < digits && i < 8) tmp[i++] = '0';
  while(i) buf[len++] = tmp[--i];
  buf[len] = 0;
  return len;
}

uint8_t Num_ToBin(uint32_t n, char *buf, uint8_t bits){
  uint8_t i;
  if(bits == 0 || bits > 32) bits = 32;
  for(i = 0; i < bits; i++){
    buf[i] = (n & (1UL << (bits - 1 - i))) ? '1' : '0';   // MSB first
  }
  buf[bits] = 0;
  return bits;
}

int32_t Num_SignExtend(uint32_t v, uint8_t bits){
  uint32_t sign = 1UL << (bits - 1);
  v &= (bits >= 32) ? 0xFFFFFFFF : ((1UL << bits) - 1);
  return (int32_t)((v ^ sign) - sign);
}

uint32_t Num_ToBCD(uint32_t n){
  uint32_t r = 0; uint8_t s = 0;
  while(n && s < 32){ r |= (n % 10) << s; n /= 10; s += 4; }
  return r;
}

uint32_t Num_FromBCD(uint32_t bcd){
  uint32_t r = 0, m = 1;
  while(bcd){ r += (bcd & 0xF) * m; m *= 10; bcd >>= 4; }
  return r;
}

uint32_t Num_ReverseBits(uint32_t v, uint8_t bits){
  uint32_t r = 0; uint8_t i;
  for(i = 0; i < bits; i++){ r = (r << 1) | (v & 1); v >>= 1; }
  return r;
}

uint32_t Num_ParseDec(const char *s){
  uint32_t n = 0;
  while(*s >= '0' && *s <= '9'){ n = n * 10 + (*s - '0'); s++; }
  return n;
}

uint32_t Num_ParseHex(const char *s){
  uint32_t n = 0;
  if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
  while(1){
    char c = *s++;
    if(c >= '0' && c <= '9')      n = (n << 4) | (c - '0');
    else if(c >= 'A' && c <= 'F') n = (n << 4) | (c - 'A' + 10);
    else if(c >= 'a' && c <= 'f') n = (n << 4) | (c - 'a' + 10);
    else break;
  }
  return n;
}

// -------------------------------------------------------------- serial ----
void Ser_Char(char c){ UART0_OutChar(c); }
void Ser_Str(const char *s){ while(*s) UART0_OutChar(*s++); }
void Ser_NL(void){ Ser_Str("\r\n"); }
void Ser_Dec(uint32_t n){ char b[12]; Num_ToDec(n, b); Ser_Str(b); }
void Ser_SDec(int32_t n){ char b[13]; Num_ToSignedDec(n, b); Ser_Str(b); }
void Ser_Hex(uint32_t n, uint8_t digits){ char b[9]; Num_ToHex(n, b, digits); Ser_Str("0x"); Ser_Str(b); }
void Ser_Bin(uint32_t n, uint8_t bits){ char b[33]; Num_ToBin(n, b, bits); Ser_Str("0b"); Ser_Str(b); }
