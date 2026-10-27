# Troubleshooting

## Build / flash
| Symptom | Cause | Fix |
|---|---|---|
| `cannot open source file "../inc/..."` | `Lab5_Toolkit` and `inc` aren't side by side | Re-import from the SC2107 root folder |
| `symbol "Bump_Read" redefined` | `Bump.c` and `BumpInt.c` both linked | Link only `BumpInt.c` |
| `symbol "SysTick_Init" redefined` | `SysTick.c` + `SysTickInts.c` | Link only one |
| `unresolved symbol TimerA1_Init` | `TimerA1.c` not linked | Add Files → `inc/TimerA1.c` → Link |
| `unresolved symbol Task_X` | Prototype or function name mismatch | Check `Tasks.h` and the spelling in `Table[]` |
| Fast red blinking at start | `TASK` number isn't in `Table[]` | Fix the number or add the row |
| Can't connect to the target | Board is in a bad state | Unplug and replug USB; hold reset while you click Debug; check the robot's power switch |
| Code changes have no effect | Old binary flashed | Project → Clean, then Build, then Debug again |

## Runtime
| Symptom | Cause | Fix |
|---|---|---|
| Nothing happens after you unplug | Program was halted at `main`, or flat batteries | Press **Resume** before unplugging; LED1 should be blinking; check battery power |
| Motors don't move but LEDs work | Robot power switch off, or batteries | Turn the RSLK power on; USB alone doesn't power the motors |
| Starts on its own | `WaitForStart` skipped (`ownStart = 1`) | Check the `Table[]` row |
| Needs 2 presses to stop | Nested `StopRequested()` loops | Inner loop: `if(StopRequested()) return;` |
| `Move_mm` stops after ~1 s | Tach not counting → stall guard | TASK 900, roll the wheels, check counts and encoder cables |
| Move overshoots | Momentum | Raise `STOP_EARLY_COUNTS` or lower the speed |
| Turns differ between runs | Slip, battery level, surface | Lower `TURN_SPEED`; recalibrate on the arena surface |
| Turns too far in one direction only | Wheel or encoder asymmetry | Calibrate left and right separately: `Turn_deg(-90 * 102 / 100, ...)` |
| Robot curves when going straight | Motor mismatch | Raise `STRAIGHT_KP`; check both tach counts rise |
| IR always 800 | Too far, or bad wiring/ADC | Check `IR_Raw` in TASK 900 against your table |
| IR jumps around | Motor noise, angled target | Read while stopped; use a flat box square to the sensor |
| IR wrong only up close (< 8 cm) | GP2Y0A21 output folds back below ~10 cm | Avoid using it below 10 cm; stop earlier |
| Line follow oscillates | Gain or speed too high | Lower `LF_KP_NUM` or `SPEED_MED` |
| Line lost on sharp curves | Gain too low or too fast | Raise `LF_KP_NUM`, or lower the speed |
| All reflectance bits 1 on white | `REF_TIME_US` too long, or the board is too far from the floor | Lower it to ~500 |
| Bump ISR fires repeatedly / CPU hangs | Old `BumpInt.c` without the IFG clear | Use the fixed `inc/BumpInt.c` |
| One press counted twice | Switch bounce | Use `Bump_Stable()`, or ignore edges < 50 ms apart |
| No serial output | Wrong COM port or baud | 115200 8N1, "XDS110 Class Application/User UART" port; `UART0_Init` must run first (Toolkit_Init) |
| Garbage on serial | Baud mismatch, or clock not at 48 MHz | 115200; make sure `Clock_Init48MHz()` runs before `UART0_Init()` |

## When stuck for more than 10 minutes
Fall back to the simpler option: open-loop `Turn_ms`/`Move_ms`, polling instead of interrupts, and bang-bang line following (recipe 10). A partially working L-task still beats an unfinished H-task: Option A/B results count if C fails.
