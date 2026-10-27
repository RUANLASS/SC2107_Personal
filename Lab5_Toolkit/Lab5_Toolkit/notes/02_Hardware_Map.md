# Hardware map (TI-RSLK, MSP432P401R): as wired in your code

## Pins
| Device | Pin(s) | Polarity / notes |
|---|---|---|
| SW1 (left button) | P1.1 | active-low, pull-up. `LaunchPad_Input()` bit0 |
| SW2 (right button) | P1.4 | active-low, pull-up. `LaunchPad_Input()` bit1 |
| Red LED1 | P1.0 | 1 = on. `LaunchPad_LED()` |
| RGB LED | P2.0 red, P2.1 green, P2.2 blue | 1 = on. `LaunchPad_Output(0..7)` |
| Reflectance IR LEDs | P5.3 | 1 = on |
| Reflectance sensors 1–8 | P7.0 – P7.7 | sensor 1 = P7.0 = **robot's right**. Charge → input → wait ~1 ms → read: **1 = black** |
| Bump0 … Bump5 | P4.0, P4.2, P4.3, P4.5, P4.6, P4.7 | active-low, pull-up. Bump0 = right, Bump5 = left. Mask `0xED` |
| Left motor | DIR P1.7, PWM P2.7 (TA0.4), nSLEEP P3.7 | DIR 0 = forward |
| Right motor | DIR P1.6, PWM P2.6 (TA0.3), nSLEEP P3.6 | DIR 0 = forward |
| Right encoder | A = P10.4 (TA3CCI0A), B = P10.5 | B high on A's rising edge = forward |
| Left encoder | A = P8.2 (TA3CCI2A), B = P9.2 | |
| IR right | P9.0 = A17 | |
| IR centre | P4.1 = A12 | |
| IR left | P9.1 = A16 | |
| UART0 | P1.2 RX, P1.3 TX | 115200 8N1 via the XDS110 USB (Application/User UART) |

## Bit maps
```
Reflectance byte:  bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0
sensor #:            8    7    6    5    4    3    2    1
robot side:        LEFT ............................ RIGHT
masks: REF_S(n)=1<<(n-1)  centre2=0x18  centre4=0x3C  outer=0x81  right half=0x0F  left half=0xF0

Bump byte (positive logic after the fix):  bit5..bit0 = Bump5..Bump0
Port4 pin:   P4.7 P4.6 P4.5 P4.3 P4.2 P4.0
Bump:          5    4    3    2    1    0
masks: BUMP(n)=1<<n  right side=0x07 (Bump0-2)  left side=0x38 (Bump3-5)

Reflectance_Position weights (sensor 1..8): +332 +237 +142 +47 -47 -142 -237 -332
  +ve = line is to the robot's RIGHT -> steer right (left wheel faster)
  no line -> 2147483647 (LINE_NONE)
```

## Clocks and timers
| Clock | Value |
|---|---|
| MCLK (CPU) | 48 MHz after `Clock_Init48MHz()` |
| SMCLK | 12 MHz |

| Timer | Used by | Tick | Period arg → time |
|---|---|---|---|
| TimerA0 | PWM (Motor.c) | 12 MHz/8 = 1.5 MHz, up-down | CCR0 = 7500 → 10 ms (100 Hz); duty = d/7500 |
| **TimerA1** (FREE) | `TimerA1_Init(&fn, period)` | 12 MHz/4/6 = 500 kHz = **2 µs** | period 500 = 1 ms, 5000 = 10 ms, 50000 = 100 ms |
| TimerA2 | Toolkit 1 ms tick | same as A1 | 500 → 1 ms |
| TimerA3 | Tachometer capture | 12 MHz = 83.3 ns | 16-bit, wraps every 5.46 ms |
| **SysTick** (FREE) | `SysTick_Init(period, prio)` (SysTickInts.c) | 48 MHz | 48000 = 1 ms, 480000 = 10 ms |
| **Timer32** (FREE) | `Timer32.c` | | |

Note: you can't link `SysTick.c` and `SysTickInts.c` together, because both define `SysTick_Init`.
`TimerA1.c` isn't linked into `Lab5_Toolkit`. To add it: right-click the project → Add Files → `inc/TimerA1.c` → **Link to files**, relative to `PARENT-1-PROJECT_LOC`.
(Bug in your `TimerA1_Stop()`: it disables IRQ 12 instead of IRQ 10. To really stop it: `TIMER_A1->CTL &= ~0x0030; NVIC->ICER[0] = 1<<10;`)

## Interrupt numbers (IRQn) and NVIC registers
| Source | IRQn | ISR name | Enable |
|---|---|---|---|
| TA0_0 | 8 | `TA0_0_IRQHandler` | `NVIC->ISER[0] = 1<<8` |
| TA1_0 | 10 | `TA1_0_IRQHandler` | `NVIC->ISER[0] = 1<<10` |
| TA2_0 | 12 | `TA2_0_IRQHandler` | `NVIC->ISER[0] = 1<<12` |
| TA3_0 / TA3_N | 14 / 15 | `TA3_0_IRQHandler` / `TA3_N_IRQHandler` | `1<<14` / `1<<15` |
| EUSCIA0 (UART) | 16 | `EUSCIA0_IRQHandler` | `1<<16` |
| ADC14 | 24 | `ADC14_IRQHandler` | `1<<24` |
| T32_INT1 | 25 | `T32_INT1_IRQHandler` | `1<<25` |
| PORT1 (SW1/SW2) | 35 | `PORT1_IRQHandler` | `NVIC->ISER[1] = 1<<(35-32)` = `0x08` |
| PORT4 (bumps) | 38 | `PORT4_IRQHandler` | `NVIC->ISER[1] = 1<<(38-32)` = `0x40` |

Priority, the safe way (`NVIC->IP` is a **byte** array, one byte per IRQ, top 3 bits used):
`NVIC->IP[35] = 2 << 5;` or `NVIC_SetPriority(PORT1_IRQn, 2);`. Lower number = higher priority.
