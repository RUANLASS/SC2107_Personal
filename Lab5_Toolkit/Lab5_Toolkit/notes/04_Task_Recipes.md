# Task recipes (copy-paste into Tasks.c)

Every recipe here compiles against `Toolkit.h`. To add one: paste it into `Tasks.c`, add the prototype to `Tasks.h`, add a `{id, Task_X, ownStart}` row in `Table[]` in main, then set `#define TASK id`.

## 0. Skeleton
```c
// LX  <task text copied from the sheet>
// USES ...   METHOD poll/ISR   PAUSE main WaitForStart / own
void Task_LX(void){
  while(!StopRequested()){
    // read inputs -> decide -> act
    Delay_ms(5);
  }
  Stop(); LED_RGB(LED_OFF); LED_Red1(0);
}
```

## 1. L-task: LED follows a sensor (manual example)
"Blink the RED LED only when a black line is below sensor #1."
```c
void Task_L_BlinkOnSensor1(void){
  while(!StopRequested()){
    if(Line_Read() & REF_S(1)){
      LED_Red1((Millis / 250) & 1);     // blink at 2 Hz, no blocking delay
    } else {
      LED_Red1(0);
    }
    Delay_ms(5);
  }
  LED_Red1(0);
}
```
Variants: RGB red → `LED_RGB((Millis/250)&1 ? LED_RED : LED_OFF)`. "Solid while black" → `LED_Red1(1)`.
Another sensor → `REF_S(n)`. "Any of" sensors 3–6 → `& REF_CENTER4`. "All of" sensors 4 and 5 → `(d & 0x18) == 0x18`.

## 2. L-task: LED follows a bump or button
```c
void Task_L_BumpLED(void){
  uint8_t b;
  while(!StopRequested()){
    b = Bump_Now();
    if(b & BUMP(3)) LED_RGB(LED_BLUE);      // Bump3 -> blue
    else if(b)      LED_RGB(LED_GREEN);     // any other -> green
    else            LED_RGB(LED_OFF);
    Delay_ms(10);
  }
  LED_RGB(LED_OFF);
}
```
If you need SW1/SW2 as **inputs**, don't use `StopRequested()` (it eats the press). Loop forever instead:
```c
void Task_L_ButtonToggle(void){            // SW1 toggles the red LED, SW2 cycles the RGB colour
  uint8_t led = 0, color = 0, sw;
  while(1){
    sw = Button_Read();
    if(sw){
      Delay_ms(20); while(Button_Read()){}  Delay_ms(20);   // debounce + release
      if(sw & SW1){ led ^= 1; LED_Red1(led); }
      if(sw & SW2){ color = (color + 1) & 7; LED_RGB(color); }
    }
  }
}
```

## 3. Wait for a condition, then act once
```c
void Task_WaitThenAct(void){
  while(!(Line_Read() & REF_S(1))){ if(StopRequested()) return; }  // wait for sensor 1 black
  Turn_deg(90, TURN_SPEED);                                         // act
}
```

## 4. Drive until X, then do a sequence
```c
static uint8_t Cond_Center4(void){ return (Line_Read() & REF_CENTER4) != 0; }
void Task_DriveUntilLine(void){
  uint8_t r = Move_mm(2000, SPEED_MED, Cond_Center4);  // max 2 m
  if(r == 1){                                          // stopped because of the line
    Delay_ms(200);
    Move_mm(-100, SPEED_MED, 0);
    Turn_deg(-90, TURN_SPEED);
  }
}
```
Without a distance limit:
```c
  Drive(SPEED_MED, SPEED_MED);
  while(IR_Center() > 200){ if(StopRequested()) return; }
  Stop();
```

## 5. React repeatedly while driving (an event loop)
```c
void Task_EventLoop(void){
  int32_t l, c, r;
  while(!StopRequested()){
    Drive(SPEED_MED, SPEED_MED);
    IR_Read(&l, &c, &r);
    if(Bump_TakeEvent() || Bump_Now()){    // event 1: bump
      Stop(); Move_mm(-100, SPEED_MED, 0); Turn_deg(180, TURN_SPEED);
    } else if(c < 150){                    // event 2: obstacle ahead
      Stop(); Turn_deg(90, TURN_SPEED);
    } else if(Line_Read() == REF_ALL){     // event 3: finish bar
      Stop(); LED_Flash(LED_GREEN, 3, 200); return;
    }
  }
}
```

## 6. Timed actions (exact seconds)
```c
  uint32_t t0 = Millis;
  Drive(SPEED_MED, SPEED_MED);
  while(Millis - t0 < 3000){}              // exactly 3 s, non-blocking style
  Stop();
```
Do something **within** a time window (e.g. count presses for 10 s): see `Task_M9_CountBumps10s`.
Non-blocking periodic work inside a loop:
```c
  uint32_t tNext = Millis;
  while(!StopRequested()){
    if((int32_t)(Millis - tNext) >= 0){ tNext += 500; P1->OUT ^= 0x01; }  // toggle LED1 every 500 ms
    /* other work */
  }
```

## 7. Counting edges (presses, lines crossed)
```c
  uint8_t prev = 0, now;  uint32_t count = 0;
  while(!StopRequested()){
    now = (Line_Read() & REF_CENTER2) != 0;   // or Bump_Stable() != 0
    if(now && !prev){ count++; LED_ShowNumber(count); }   // rising edge only
    prev = now;
    Drive(SPEED_SLOW, SPEED_SLOW);
    Delay_ms(5);
  }
```
Counting black lines crossed while driving: use `SPEED_SLOW`, and require 2 consecutive readings so it isn't fooled by noise.

## 8. Finite state machine
```c
typedef enum {S_FWD, S_BACK, S_TURN, S_DONE} St_t;
void Task_FSM_Template(void){
  St_t s = S_FWD;  uint32_t tEnter = Millis;  St_t prev = S_DONE;
  while(!StopRequested()){
    if(s != prev){ prev = s; tEnter = Millis; LED_RGB(s + 1); }   // entry actions
    switch(s){
      case S_FWD:  Drive(SPEED_MED, SPEED_MED);
                   if(Bump_Now()) s = S_BACK;
                   break;
      case S_BACK: Drive(-SPEED_MED, -SPEED_MED);
                   if(Millis - tEnter > 500) s = S_TURN;
                   break;
      case S_TURN: Turn_deg(90, TURN_SPEED); s = S_FWD;      break;
      case S_DONE: Stop();                                   break;
    }
    Delay_ms(2);
  }
}
```

## 9. Bump number → conversion → serial (the classic H-task)
Already done in `Task_H1_BumpBinDecHex` (TASK 201). Variants:
- Octal: write the digits with `n % 8` and `n / 8`, the same way as `Num_ToDec` (see `06_Number_Conversion.md`).
- Two's complement signed: `Ser_SDec(Num_SignExtend(v, 6))`.
- BCD: `Ser_Hex(Num_ToBCD(v), 2)`.
- Show on LEDs instead: `LED_ShowNumber(v)` (0–15) or `LED_BlinkCount(v, LED_WHITE)`.

## 10. Line following variants
- Basic: `Line_FollowStep(SPEED_MED)` in a loop with `Delay_ms(2)`.
- With recovery: `Follow_Step(base, lostMs)` (static helper in `Tasks.c`).
- Bang-bang fallback (if P-control misbehaves):
```c
  uint8_t d = Line_Read();
  if(d & REF_RIGHTHALF & ~REF_CENTER2)     Drive(SPEED_MED, SPEED_SLOW/2);  // line right -> turn right
  else if(d & REF_LEFTHALF & ~REF_CENTER2) Drive(SPEED_SLOW/2, SPEED_MED);  // line left  -> turn left
  else if(d)                               Drive(SPEED_MED, SPEED_MED);
```

## 11. Speed from the tachometer
```c
  Drive(SPEED_MED, SPEED_MED);
  Delay_ms(1000);                          // let it settle
  Ser_Str("L mm/s = "); Ser_SDec(SpeedL_mms); Ser_Str("  R mm/s = "); Ser_SDec(SpeedR_mms); Ser_NL();
  // rpm = counts per minute / 360 = mm_s * 60 / 220
```
A target speed: `Drive_AtSpeed(150, 3000);` (150 mm/s for 3 s).

## 12. Distance travelled readout
```c
  Tach_Reset();
  Drive(SPEED_MED, SPEED_MED);
  while(!Bump_Now()){}                     // until something is hit
  Stop();
  Ser_Str("Travelled "); Ser_SDec(Tach_DistanceMM()); Ser_Str(" mm"); Ser_NL();
```
