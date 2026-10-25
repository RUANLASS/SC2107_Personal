# Lab5 Toolkit

A CCS project that builds on your own Lab1–4 drivers in `../inc`. It gives you ready-made primitives and a worked version of every practice M/H task, so on the day you copy the closest task and edit it instead of starting from scratch.

> **None of this has been run on a robot yet.** It compiles and links with an ARM compiler against the MSP432 headers, but the timing, turns, IR tables and line-follow gain have to be tuned on **your** RSLK. Do the checklist below before week 11.

## Import into CCS
1. Copy this `Lab5_Toolkit` folder into your SC2107 workspace, next to `inc/`. The folders must be siblings: the project links to `../inc/*.c`.
2. Use the updated `inc/` files too (see *Driver fixes*).
3. In CCS, go to **File › Import › CCS Projects**, select the SC2107 folder and tick `Lab5_Toolkit`.
4. Set `#define TASK` in `Lab5_Toolkitmain.c`, then Build, Debug and Resume.

## Running a task away from the desk
- **LED1 blinking** means it's waiting. Unplug the USB and place the robot.
- Press **SW1 or SW2** to start. LED1 stays solid for 1 s, then the task runs.
- Press **SW1/SW2** during a looping task to stop it. LED1 blinks again, so you can retry without re-flashing.
- Tasks M4, M9 and H4 wait for their own button (SW2, SW1, SW2), as the task text says.

## TASK numbers
| TASK | What | Where |
|---|---|---|
| 0 | **Pre-known demo**: watch `PK_Reflect`, `PK_BumpNum`, `PK_Bump`, `IR_mm` | desk |
| 900 | Calibration stream on serial: IR raw/mm, reflectance, bumps, tach counts | desk |
| 901 / 902 / 903 / 904 | Square test / 360° turn / 1 m straight / line follow | floor |
| 101–110 | M1–M10 | |
| 201–210 | H1–H10 | |

## Calibrate BEFORE Lab5 (the constants are in `Toolkit.h`)
1. **IR tables** (`inc/IRDistance.c`): run TASK 900 and hold a box at 10, 15, 20, 25, 30 and 40 cm in front of each sensor. Type the raw values into `IRLeftTable` / `IRCenterTable` / `IRRightTable`, with ADC values descending. If you prefer a curve fit, fit `d = A/(n+B)+C` and set `IR_USE_FIT 1`. Online curve-fitting tools are allowed during the lab.
2. **Turns**: run TASK 902. If it turns less than 360°, raise `TURN_CAL_PCT`; if more, lower it. Then run TASK 901: the square should close.
3. **Distance**: run TASK 903 and tape-measure the result. Set `DIST_CAL_PCT = 100 × 1000 / measured`.
4. **Line follow**: run TASK 904. If it wobbles, lower `LF_KP_NUM`; if it loses the line on curves, raise it. Re-tune `SPEED_MED` with it.
5. **Pre-known**: run TASK 0 and confirm that all 8 reflectance bits, all 6 bumps and 20 cm ±5 on L/C/R read correctly.

## Primitive cheat sheet
| Need | Call |
|---|---|
| Start / pause | `WaitForStart()`, `WaitForButton(SW1)`, `TimedPause(ms)`, `StopRequested()` |
| Time | `Millis`, `Delay_ms(ms)` (1 ms tick on TimerA2) |
| LEDs | `LED_RGB(LED_RED)`, `LED_Red1(1)`, `LED_Flash(c,n,ms)`, `LED_ShowNumber(0..15)`, `LED_BlinkCount(n,c)` |
| Reflectance | `Line_Read()` (bit = black, `REF_S(1)` = sensor #1 = robot's right), `Line_Position()`, `Line_FollowStep(base)` |
| Bumps | `Bump_Now()` (1 = pressed), `Bump_Stable()`, `Bump_TakeEvent()` (interrupt flag), `Bump_BothEdges(1)`, `Bump_LowestIndex()` |
| Motors | `Drive(l, r)` (signed), `Stop()`, `Move_ms(l,r,ms)`, `Turn_ms(deg)` |
| Tachometer | `Move_mm(±mm, duty, cond)`, `Turn_deg(±deg, duty)` (+ = right), `Tach_Reset/Left/Right/DistanceMM()`, `SpeedL_mms`, `Drive_AtSpeed(mm/s, ms)` |
| Stop conditions | `Cond_Bump`, `Cond_AnyBlack`, `Cond_AllWhite`, `Cond_IRClose` (`IR_CloseMM`) |
| IR | `IR_Read(&l,&c,&r)`, `IR_Left/Center/Right()` in mm, 800 = nothing there |
| Numbers (no library) | `Num_ToDec/ToSignedDec/ToHex/ToBin`, `Num_SignExtend`, `Num_ToBCD/FromBCD`, `Num_ReverseBits`, `Num_ParseDec/ParseHex`, `Count_Ones` |
| Serial 115200 | `Ser_Str`, `Ser_Dec`, `Ser_SDec`, `Ser_Hex(n,digits)`, `Ser_Bin(n,bits)`, `Ser_NL` |

The free timers for your own ISR are **TimerA1**, **SysTick** and **Timer32**.

## Task skeleton summary
| Task | Sensors / outputs | Method | Key calls | Start |
|---|---|---|---|---|
| M1 line → left 90° | refl #3, motors | poll | `Line_Read`, `Turn_deg(-90)` | SW1/2 |
| M2 bump → reverse, turn away | bumps, motors | ISR flag + poll | `Bump_TakeEvent`, `Move_ms`, `Turn_deg` | SW1/2 |
| M3 IR<15 → reverse 10, right 90 | IR C, tach | poll | `IR_Center`, `Move_mm(-100)` | SW1/2 |
| M4 SW2 → follow 5 s | SW2, refl | poll | `Follow_Step`, `Millis` | **SW2** |
| M5 bumps0-2 → LED | bumps, RGB | poll | `Bump_Stable`, `LED_RGB` | SW1/2 |
| M6 refl → red/green | refl, RGB | poll | `REF_CENTER4`, `REF_OUTER` | SW1/2 |
| M7 left wheel 30 cm | tach | poll | `Tach_Left`, `MM_ToCounts` | SW1/2 |
| M8 corridor | IR L/R | poll | `IR_Read`, `Drive` | SW1/2 |
| M9 count bumps 10 s | SW1, bumps, LEDs | poll | `Bump_Stable`, `LED_ShowNumber` | **SW1** |
| M10 #1 black → 2 s | refl #1 | poll + `Millis` | `REF_S(1)` | SW1/2 |
| H1 bump → bin/dec/hex | bumps, UART | **GPIO ISR** both edges | `Num_ToDec/Hex/Bin` | SW1/2 |
| H2 follow + recovery + park | refl | poll | `Follow_Step`, `Count_Ones` | SW1/2 |
| H3 avoid + count | IR, tach, LED, UART | poll | `IR_Read`, `Move_mm`, `Turn_deg` | SW1/2 |
| H4 180° + 40 cm | SW2, tach | poll | `Turn_deg(180)`, `Move_mm(400)` | **SW2** |
| H5 4-state FSM | refl, IR, bumps | FSM | `switch(state)` | SW1/2 |
| H6 bump speed/dir | bumps | poll | `Bump_Stable`, `Drive` | SW1/2 |
| H7 25 cm sequence | IR C, LEDs, tach | poll | `IR_Center`, `LED_Flash` | SW1/2 |
| H8 line + bump override | refl, bumps | ISR flag + poll | `Follow_Step`, `Turn_deg(±45)` | SW1/2 |
| H9 parking | refl, tach | phases | `Cond_AllWhite`, `Cond_AnyBlack` | SW1/2 |
| H10 bump number → drive cm | bumps, SW1/2, tach | poll | `Move_mm(value*10)` | SW1/2, then SW1 = go |

Every task in `Tasks.c` has an **ASK** line listing what to clarify with the instructor first, for example which way to turn, or whether a value is latched or held.

## Driver fixes made in `../inc` (the toolkit depends on them)
- **`IRDistance.c`**: the 10–20 cm segment used the 5–10 cm end points, so **an obstacle at 20 cm read as about 29 cm**, which would fail the Lab4 pre-known check. Readings outside the table returned 0, which looks like "touching" to obstacle-avoidance code; far readings now return 800. There was also a 2824/2842 typo gap. The file now has one table per sensor and an optional curve fit.
- **`Bump.c`, `BumpInt.c`**: `Bump_Read()` returned raw negative logic (0x3F with nothing pressed). It now returns positive logic (Bump0 → 0x01), as its header says.
- **`BumpInt.c`**: the ISR now always clears `P4->IFG`, so a task that forgets can't hang the CPU. It also has an optional interrupt on both press and release (`BumpInt_BothEdges`).
- **`Reflectance.c`**: `Reflectance_End()` cleared the whole of `P5->OUT`; it now clears only P5.3.
- **`Lab4_ADCmain.c`**: added `DistL/DistC/DistR` (mm) for the watch window. Before, it only showed filtered ADC values.

Not changed, but worth knowing:
- `TimerA1_Stop()` disables NVIC IRQ 12 instead of 10, so it doesn't actually disable TimerA1's interrupt.
- In `Lab4_Tachmain.c`, `if(main_count%1000)` prints on 999 of every 1000 loops; it should be `== 0`.
