# Lab5 Notes: index

Open these in CCS (double-click a `.md` file) or any text editor.

| File | Use it when |
|---|---|
| `01_DayOf_Procedure.md` | At the start: timeline, the per-task routine, questions to ask, submission |
| `02_Hardware_Map.md` | You need a pin, a bit mask, a polarity or an IRQ number |
| `03_Toolkit_API.md` | You need the exact name or arguments of a toolkit function |
| `04_Task_Recipes.md` | You're writing a new task: copy-paste templates for each pattern |
| `05_Registers_Interrupts.md` | A task forces "from scratch" GPIO, interrupt, timer or ADC code |
| `06_Number_Conversion.md` | Lab1-style conversions: the algorithms and a 0–63 lookup table |
| `07_Calibration_Record.md` | Your measured constants (fill in during practice) |
| `08_Troubleshooting.md` | Something doesn't work |
| `09_Task_Code_Explained.md` | You need to understand or explain how a task in Tasks.c works |

## The 10 facts to never get wrong
1. Reflectance **bit = 1 means BLACK**. Sensor #1 = bit0 = P7.0 = the **robot's right**; sensor #8 = bit7 = left. `REF_S(n)` = `1 << (n-1)`.
2. Bumps (after the fix) use **positive logic**: Bump0 = 0x01 (right) … Bump5 = 0x20 (left). The hardware itself is active-low with pull-ups.
3. SW1 = P1.1 → `0x01`, SW2 = P1.4 → `0x02` from `LaunchPad_Input()`. Both are active-low.
4. `Turn_deg(+90)` = **right**/clockwise, `-90` = left. `Move_mm(+)` = forward.
5. `Drive(left, right)`: signed duty from −7000 to +7000. PWM period is 7500.
6. IR returns mm; **800 = nothing seen**. ADC channels: 17 = right (P9.0), 12 = centre (P4.1), 16 = left (P9.1).
7. 90° pivot ≈ 180 tach counts per wheel; 1 count ≈ 0.611 mm; 360 counts = 1 wheel turn ≈ 220 mm.
8. Timers in use: A0 = PWM, A2 = 1 ms tick, A3 = tach. **Free: TimerA1, SysTick, Timer32.**
9. Never call `Delay_ms` with interrupts disabled: it counts on the TimerA2 tick.
10. For a new task, **press Resume before you unplug**, then press SW1/SW2 at the arena.
