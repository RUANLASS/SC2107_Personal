# Toolkit API (Toolkit.h)

## Core and time
| Call | Does |
|---|---|
| `Toolkit_Init()` | Clock 48 MHz, LEDs and switches, motors, reflectance, bump ISR, tach, ADC, UART, 1 ms tick; enables interrupts |
| `Millis` | ms since init (volatile `uint32_t`). Elapsed: `Millis - t0` |
| `Delay_ms(ms)` | accurate delay (needs interrupts enabled) |
| `Clock_Delay1ms(n)` / `Clock_Delay1us(n)` | busy-wait, works with interrupts off, less accurate |

## Buttons and start
| Call | Does |
|---|---|
| `Button_Read()` | live: 0, `SW1`, `SW2`, `SW1\|SW2` |
| `WaitForButton(mask)` | blinks LED1, waits for press **and release**, returns which |
| `WaitForStart()` | `Stop()` + `WaitForButton(SW1\|SW2)` + 1 s delay. main calls it before tasks with `ownStart = 0` |
| `TimedPause(ms)` | course version: run `ms`, stop motors, wait for press then release |
| `StopRequested()` | 1 if SW1/SW2 is pressed now (stops motors, waits for release). Use: `while(!StopRequested()){…}` |

## LEDs
| Call | Does |
|---|---|
| `LED_RGB(c)` | `LED_OFF RED GREEN YELLOW BLUE PINK SKYBLUE WHITE` (0–7: bit0 = R, bit1 = G, bit2 = B) |
| `LED_Red1(on)` | LED1 (P1.0) |
| `LED_Flash(c, n, ms)` | flash RGB colour + LED1 `n` times |
| `LED_ShowNumber(n)` | 0–15 in binary: RGB = bits 0–2, LED1 = bit 3 |
| `LED_BlinkCount(n, c)` | blink `n` times (0 = one long blink) |

## Reflectance
| Call | Does |
|---|---|
| `Line_Read()` | 8 bits, 1 = black (`Reflectance_Read(REF_TIME_US)`, ~1 ms) |
| `Line_Position(d)` | −332 … +332, + = line on the right; `LINE_NONE` if none |
| `Line_FollowStep(base)` | one P-control step, returns data (0 = lost, motors unchanged) |
| `Count_Ones(v)` | number of 1 bits (e.g. how many sensors see black) |
| masks | `REF_S(n) REF_CENTER2 REF_CENTER4 REF_OUTER REF_RIGHTHALF REF_LEFTHALF REF_ALL` |

## Bumps
| Call | Does |
|---|---|
| `Bump_Now()` | 6 bits, 1 = pressed, no debounce |
| `Bump_Stable()` | debounced (same value twice, 15 ms apart) |
| `Bump_TakeEvent()` | 1 if the PORT4 ISR fired since the last call (clears the flag) |
| `Bump_BothEdges(1)` | ISR fires on press **and** release (for "on every change") |
| `Bump_LowestIndex(b)` | 0–5 of the lowest pressed bump, −1 if none |
| masks | `BUMP(n) BUMP_RIGHTSIDE (0x07) BUMP_LEFTSIDE (0x38)` |

## Motors
| Call | Does |
|---|---|
| `Drive(l, r)` | signed duty per wheel (clamped to ±`MOTOR_MAX`) |
| `Stop()` | both motors off (coast) |
| `Move_ms(l, r, ms)` | drive for a time, then stop |
| `Turn_ms(deg)` | open-loop turn using `MS_PER_90DEG` (fallback) |
| speeds | `SPEED_SLOW 1500`, `SPEED_MED 2500`, `SPEED_FAST 4000`, `TURN_SPEED 2000` |

Arcs/curves: `Drive(SPEED_MED, SPEED_MED/3)` curves **right**; `Drive(SPEED_MED/3, SPEED_MED)` curves **left**.
Pivot: `Drive(s, -s)` = spin right, `Drive(-s, s)` = spin left.

## Tachometer moves
| Call | Does |
|---|---|
| `Move_mm(mm, duty, cond)` | ± mm, straight-line correction. Returns 0 done, 1 `cond` true, 2 stalled 1 s |
| `Turn_deg(deg, duty)` | pivot, + = right. 5 s safety timeout |
| `Drive_AtSpeed(mm_s, ms)` | closed-loop speed for a duration |
| `Tach_Reset()` `Tach_Left()` `Tach_Right()` | absolute counts since reset |
| `Tach_DistanceMM()` | average distance since reset |
| `MM_ToCounts(mm)` | e.g. 300 → ~491 |
| `SpeedL_mms` `SpeedR_mms` | wheel speeds, updated every 100 ms |
| conditions | `Cond_Bump`, `Cond_AnyBlack`, `Cond_AllWhite`, `Cond_IRClose` (+ `IR_CloseMM = 200`) |

A custom stop condition:
```c
static uint8_t Cond_Sensor8(void){ return (Line_Read() & REF_S(8)) != 0; }
Move_mm(1000, SPEED_SLOW, Cond_Sensor8);   // drive up to 1 m, stop when sensor 8 sees black
```

## IR
| Call | Does |
|---|---|
| `IR_Read(&l, &c, &r)` | mm, averaged over 32 samples (~0.5 ms). Pass 0 for any you don't need |
| `IR_Left()` `IR_Center()` `IR_Right()` | single sensor in mm |
| `IR_Raw[3]`, `IR_mm[3]` | last values [L, C, R] for the watch window |

## Numbers and serial (no library)
| Call | Example |
|---|---|
| `Num_ToDec(n, buf)` | 43 → "43" |
| `Num_ToSignedDec(n, buf)` | −5 → "-5" |
| `Num_ToHex(n, buf, digits)` | 43, 2 → "2B" |
| `Num_ToBin(n, buf, bits)` | 43, 6 → "101011" |
| `Num_SignExtend(v, bits)` | 0x3F, 6 → −1 |
| `Num_ToBCD` / `Num_FromBCD` | 43 ↔ 0x43 |
| `Num_ReverseBits(v, bits)` | 0b001, 3 → 0b100 |
| `Num_ParseDec` / `Num_ParseHex` | "123" → 123, "0x2B" → 43 |
| `Ser_Str Ser_Char Ser_Dec Ser_SDec Ser_Hex(n,d) Ser_Bin(n,b) Ser_NL` | print via UART0 |

Buffer sizes: dec 11, signed dec 12, hex 9, bin 33 chars (including the NUL).
