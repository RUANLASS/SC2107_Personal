# Task code explained (Tasks.c + main)

For each task: **what it does → how the code flows → the lines that matter → how to vary it**.
All of them rely on the toolkit primitives in `03_Toolkit_API.md`.

---

## The framework (Lab5_Toolkitmain.c)

```c
#define TASK 0
static const TaskEntry Table[] = { {id, function, ownStart}, ... };
```
1. `Toolkit_Init()` sets up everything (clock, motors, sensors, bump ISR, tach, ADC, UART, 1 ms tick).
2. It looks up `TASK` in `Table[]`. If the number isn't there, the RGB LED blinks red fast.
3. Then it loops forever:
   - `ownStart = 0` → `WaitForStart()` first (LED1 blinks, press SW1/SW2, 1 s delay).
   - Run the task function.
   - `Stop()`, LEDs off, and loop back, so you can **retry without re-flashing**.
   - `ownStart = 1` (TASK 0, 900, M4, M9, H4): the task waits for its own button, and main waits for a press after it ends.

**`StopRequested()`** shows up in almost every loop: `while(!StopRequested()){ ... }`. Pressing SW1/SW2 stops the motors, waits for release, returns 1, the loop exits, and you're back in main waiting to start again.
**Nested loops:** use `if(StopRequested()) return;` in the inner loop. Otherwise the outer loop carries on and you need a second press.

### Shared helper: `Follow_Step(base, lostMs)`
```c
data = Line_FollowStep(base);           // P-control step; 0 if no line
if(data){ LastSeen = Millis; LastPos = Line_Position(data); return data; }
if(Millis - LastSeen > lostMs) spin toward LastPos side;
```
- Line visible → steer with `corr = position × LF_KP_NUM`, so `Drive(base + corr, base − corr)`. Line on the right (positive position) → left wheel faster → turns right.
- Line lost for less than `lostMs` → motors keep their last command (coasting through gaps).
- Lost for longer → spin in place toward the side the line was last seen.
- Call `Follow_Reset()` before starting so the timer starts from "now".

---

## Desk tasks

### TASK 0: Pre-known demo (`Task_PreKnown`)
An endless loop: `PK_Reflect = Line_Read()`, `PK_Bump = Bump_Now()`, `PK_BumpNum = Bump_LowestIndex(PK_Bump)`, `IR_Read()` fills `IR_mm[]`. The RGB LED is blue when a bump is pressed and green when any sensor sees black. Every 500 ms it also prints all values on serial. There's no stop, and it's meant for the watch window.

### TASK 900: Calibration (`Task_Calibrate`)
`Tach_Reset()`, then every 300 ms prints the raw and mm IR values, reflectance bits, bump bits, tach counts and distance. Use it to fill the IR tables and to check the encoders by rolling the wheels by hand.

### TASK 901–904: Tests
- 901 `Test_Square`: 4 × (`Move_mm(300)` + `Turn_deg(90)`). If the square doesn't close, the turn or distance calibration is off.
- 902 `Test_Turn360`: one `Turn_deg(360)`. Tunes `TURN_CAL_PCT`.
- 903 `Test_Straight1m`: `Move_mm(1000)`. Tunes `DIST_CAL_PCT` and `STRAIGHT_KP`.
- 904 `Test_LineFollow`: `Follow_Step(SPEED_MED, 300)` until stopped. Tunes `LF_KP_NUM`.

---

## M-tasks

### M1 (101): Sensor #3 black → turn 90° left, continue
```c
Drive(MED, MED);
if(Line_Read() & REF_S(M1_SENSOR)){ Stop(); Turn_deg(-90); Drive(SLOW, SLOW);
   while(Line_Read() & REF_S(M1_SENSOR)) { if(StopRequested()) return; } }
```
- `REF_S(3)` = 0x04, the mask for sensor 3.
- `Turn_deg(-90)`: a negative angle turns left.
- **Why the inner while:** after turning, the sensor may still be over the tape. Without it, the robot would turn again immediately. It creeps forward until the sensor sees white, which re-arms the trigger.
- Vary it: `M1_SENSOR 1` and `Turn_deg(+90)` gives the manual's example (turn right on sensor #1). For "only once", put `return;` after the turn.

### M2 (102): Bump → reverse 0.5 s → turn away from the hit side
- The PORT4 interrupt sets a flag; `Bump_TakeEvent()` reads and clears it. `|| Bump_Now()` also catches a switch still being held.
- `hit = Bump_Now()` is captured **immediately**, before the bumper springs back.
- `Move_ms(-MED, -MED, 500)` = a timed reverse.
- Side decision: `Count_Ones(hit & 0x07)` (right side, Bump0–2) vs `Count_Ones(hit & 0x38)` (left side). Hit on the right → `Turn_deg(-90)` (left), and vice versa.
- `Bump_TakeEvent()` at the end discards bounces from the manoeuvre itself.
- Vary it: a reverse distance instead of a time → `Move_mm(-150, SPEED_MED, 0)`.

### M3 (103): Centre IR < 15 cm → stop, reverse 10 cm, turn 90° right
A loop: drive, `IR_Center() < 150` → `Stop()`, `Move_mm(-100)`, `Turn_deg(90)`, continue. The short `Delay_ms(200)` gaps let the robot settle so the moves are accurate.
Vary it: change `M3_STOP_MM`; use `IR_Left()`/`IR_Right()`, or `IR_Read(&l,&c,&r)` and test whichever sensor you need.

### M4 (104): Wait for SW2 → line-follow for 5 s → stop (ownStart)
`WaitForButton(SW2)` blocks until SW2 is pressed and released (SW1 is ignored). Then `t0 = Millis; while(Millis - t0 < 5000) Follow_Step(...)`. Using `Millis` gives an accurate 5 s no matter how long each loop takes.

### M5 (105): Bump0–2 as a 3-bit number → LEDs
`v = Bump_Stable() & 0x07`, keeping only bits 0–2. `LED_RGB(v)` maps the value straight to a colour (1 red, 2 green, 3 yellow, 4 blue, 5 pink, 6 sky, 7 white). When `v` changes, it prints the binary and decimal value. With `M5_BLINK 1`, it blinks `v` times instead (this blocks, so it only shows on change).

### M6 (106): Middle four black → red; only outer black → green
`d & REF_CENTER4` (0x3C = sensors 3–6) → red RGB + LED1. Otherwise `d & REF_OUTER` (0x81 = sensors 1 and 8) → green. Otherwise off. The order of the `if`s gives "middle wins", which is what makes it "only outer".

### M7 (107): Drive until the LEFT wheel has turned 30 cm worth of counts
`target = MM_ToCounts(300) − STOP_EARLY_COUNTS` (~488). The loop runs while `Tach_Left() < target`, applying `corr = (Left − Right) × STRAIGHT_KP` so it goes straight (left ahead → slow left). It prints the counts at the end.
Difference from `Move_mm`: this one checks only the left wheel, as the task says. `Move_mm` uses the average of both.

### M8 (108): Corridor centring with left/right IR
- `l + 50 < r` → closer to the left wall → `Drive(MED + 800, MED − 800)` (veer right).
- `r + 50 < l` → veer left. Otherwise straight.
- The 50 mm dead band stops constant zig-zagging. Centre IR < 100 or any bump → stop (a safety check).
- Tune: `M8_STEER` sets how hard it corrects; `M8_DEADBAND_MM` sets the tolerance.

### M9 (109): Wait for SW1, count bump presses for 10 s, display (ownStart)
`prev`/`now` edge detection: `if(now && !prev) count++`. This counts presses, not "time held". `Bump_Stable()` debounces, so one press isn't counted twice. `LED_ShowNumber(count)` shows 0–15 in binary (RGB = bits 0–2, LED1 = bit 3). After 10 s it prints the count and keeps it on the LEDs until a button is pressed.

### M10 (110): Sensor #1 black → drive exactly 2 s
`M10_MODE 0`: the robot sits still until sensor 1 sees black (slide the tape under it). `M10_MODE 1`: it creeps forward until sensor 1 sees black. Then `Drive` + `while(Millis − t0 < 2000)` + `Stop()`.

---

## H-tasks

### H1 (201): Bump bits → binary / decimal / hex on serial, GPIO interrupt, no library
1. `Bump_BothEdges(1)`: the PORT4 ISR fires on press **and** release, because the task says "on every change".
2. The ISR only sets a flag. The main loop checks `Bump_TakeEvent()`.
3. `Bump_Stable()` waits for the bounce to settle, then reads the 6-bit value.
4. If the value differs from `last`, it converts with **its own digit loops**: `Num_ToBin(v, bin, 6)`, `Num_ToDec`, `Num_ToHex(v, hex, 2)`. It prints `Binary: 101011   Decimal: 43   Hex: 0x2B`.
5. `last = 0xFF` at the start forces one initial print.
6. On exit it calls `Bump_BothEdges(0)` to restore press-only.
- The ISR stays short and the work happens in main, because you mustn't print or delay inside an ISR.
- Vary it: octal (see `06_Number_Conversion.md`), signed `Num_SignExtend(v, 6)`, BCD `Num_ToBCD(v)`.

### H2 (202): Line follow + recover after 1 s lost + park on the all-black bar
`d = Follow_Step(MED, 1000)` handles both following and the "lost > 1 s → spin" recovery. The end bar is `Count_Ones(d) >= 8` for **3 readings in a row** (`endCount`), so that one noisy reading or a quick cross-line doesn't stop it. When the bar is confirmed: `Stop()` and flash white 3×.
The RGB is green while it sees the line and red while lost, which helps you debug.

### H3 (203): Avoid obstacles < 20 cm and count them
Each loop: drive, `IR_Read(&l,&c,&r)`. If any reading is below its threshold → stop, `Move_mm(-100)`, then turn **away**: `l < r` (obstacle on the left) → `Turn_deg(+90)` right, otherwise left. `count++` → `LED_ShowNumber(count)`, plus a serial line with the count and the three distances.
Tune: `H3_SIDE_MM` separately from `H3_FRONT_MM`, because the side sensors see walls at an angle.

### H4 (204): SW2 → precise 180° turn (tach) → forward 40 cm (ownStart)
`WaitForButton(SW2)`, a 0.5 s pause, `Turn_deg(180)`, a 0.3 s settle, `Move_mm(400)`. It prints the measured distance.
**How `Turn_deg` works:** counts per wheel = `deg × TRACK_WIDTH / WHEEL_DIAMETER` (π cancels) = 180° × 140/70 = 360, minus `STOP_EARLY_COUNTS`. The wheels spin in opposite directions, and **each wheel stops on its own** when it reaches its count, so a faster wheel doesn't over-rotate.

### H5 (205): 4-state FSM (SEARCH / FOLLOW / AVOID / STOP)
Each loop iteration:
1. **Global transitions first, highest priority:** ≥ 2 bumps on one side → STOP. Else, if in SEARCH/FOLLOW and centre IR < 150 → AVOID.
2. **Entry action:** when `state != prev`, print `State -> X` and set its colour (blue / green / red / white).
3. **State behaviour (`switch`):**
   - SEARCH: spin; any line seen → FOLLOW.
   - FOLLOW: `Line_FollowStep`; lost for > 300 ms → SEARCH.
   - AVOID: stop, reverse 10 cm, turn 90° right, forward 15 cm (stops early on a bump) → SEARCH.
   - STOP: motors off and stays there (only SW1/SW2 exits).
- In an exam, explain it as a state table: state → output → transition condition → next state.

### H6 (206): Bumps choose speed (Bump0–2) and direction (Bump3–5)
Priority by `if/else`: Bump2 → fast, Bump1 → medium, Bump0 → slow. Bump3 → forward, Bump4 → spin left, Bump5 → spin right.
- `H6_LATCH 0`: nothing held → stop (the motors follow what's held right now).
- `H6_LATCH 1`: remembers the last choice, so you can tap and let go.
- The RGB shows the speed level. Ask the instructor which behaviour they expect.

### H7 (207): Stop at 25 cm ± 3, flash 3×, reverse 15 cm, turn 90° left
A closed-loop approach: too far → creep forward, too close → creep back, inside 220–280 mm → stop. It needs **3 readings in a row** inside the band (`inBand`), so it doesn't stop on a noisy reading. Then it runs the sequence: `LED_Flash(WHITE, 3)`, `Move_mm(-150)`, `Turn_deg(-90)`.
It creeps at `SPEED_SLOW` because at speed it would overshoot the 6 cm band between readings.

### H8 (208): Line following, but a bump overrides it
Normal loop: `Follow_Step(MED, 500)`. On a bump event: stop, `hit = Bump_Stable()`, `Move_mm(-100)`, turn **45°** away from the hit side (same right/left count logic as M2), clear any pending event, `Follow_Reset()`, and resume. If the line is now out of view, `Follow_Step`'s spin recovery finds it again.

### H9 (209): Park: follow to the wide bar → turn 90° → drive into the bay → stop on the bay line
Three phases in sequence:
1. Follow the line until `d == REF_ALL` (all 8 black) three times in a row.
2. `Turn_deg(H9_TURN_DEG)`.
3. `Move_mm(600, SLOW, Cond_AllWhite)` drives until it's **off** the black (the sensors may still be on the bar after the turn). Then `Move_mm(600, SLOW, Cond_AnyBlack)` drives until the bay line is under the sensors, giving up after 600 mm.
4. Optional `H9_EXTRA_MM`: the sensor bar is ahead of the wheels, so drive a little further to centre the body over the line. Measure this on your robot.

### H10 (210): Enter a number 0–63 on the bumps → SW1 → drive that many cm
- `newly = now & ~prev` = bumps that **just** went down. `value ^= newly` toggles those bits, so tapping a bump flips its bit and you don't have to hold all six at once.
- Every change prints `Value: 0b001010 = 10 cm`. The RGB shows the low 3 bits.
- SW2 → `value = 0`. SW1 → leave the loop, 1 s pause, `Move_mm(value × 10)`, print the measured mm.

---

## Patterns you'll reuse
| Pattern | Where | Core lines |
|---|---|---|
| Drive and react | M1, M2, M3, H3 | `while(!StopRequested()){ Drive(); if(cond){ Stop(); sequence; } }` |
| Edge detect (count presses) | M9, H10 | `if(now && !prev) ...; prev = now;` |
| Debounce | M5, M9, H1, H6 | `Bump_Stable()` |
| ISR flag → main | M2, H1, H8 | `Bump_TakeEvent()` |
| Accurate timing | M4, M10 | `t0 = Millis; while(Millis - t0 < T)` |
| N-in-a-row confirm | H2, H7, H9 | `if(cond){ if(++n >= 3) act; } else n = 0;` |
| Tach-based moves | M3, H3, H4, H9 | `Move_mm`, `Turn_deg` |
| State machine | H5 | transitions → entry action → `switch(state)` |
| Side decision | M2, H8 | `Count_Ones(hit & 0x07)` vs `Count_Ones(hit & 0x38)` |
