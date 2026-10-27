# Day-of procedure

## Timeline (measured from the official start, with no extra time for arriving late)
| Time | Do |
|---|---|
| 0:00–0:10 | Copy SC2107 to the lab PC (thumb drive or cloud), import projects, build `Lab5_Toolkit` |
| 0:10–0:30 | Pre-known demos: Lab2, Lab3, Lab4. Get each one signed off as soon as it works |
| by 1:00 | **Hard deadline for the pre-known demos** |
| 0:30 | Read all of the L/M/H task sheet. Pick **Option C (L + H)** unless the H task looks impossible |
| 0:30–1:00 | L-task first (quick 20%), then the H-task. **Option C done by 1:00 = +10% bonus** |
| by 2:00 | All L/M/H tasks done. Try the M-task too if time allows, since the best option counts |
| end | Zip the SC2107 folder (~100 MB), upload it to NTULearn, hand in the checklist |

Options: A = 2L (40%), B = L + M (50%), C = L + H (70%). Retries are allowed with no penalty.

## Pre-known demos (what to show)
| Demo | Run | Watch | Show |
|---|---|---|---|
| Lab2 | `Lab2_ReflectanceGPIO` | `Data` | Each of the 8 sensors over tape sets bit n−1 |
| Lab3 | `Lab3_Bump_Reflectance_Systick` | `bump_data` | All 6 bumps, one at a time: 0x01 … 0x20 |
| Lab4 | `Lab4_ADC_IRSensors` | `DistL DistC DistR` | Box at 20 cm in front of L, C and R reads within ±5 cm |
| Backup | `Lab5_Toolkit` TASK 0 | `PK_Reflect PK_BumpNum IR_mm` | All three of the above in one program |

Watch window: **View › Expressions**, add the variable, and turn on continuous refresh (the timer icon).

## Routine for every task
1. **Read the task twice.** Underline every number, sensor number, direction, colour and time.
2. **Ask the instructor** about anything ambiguous (list below). Their reading of the task is final.
3. Find the closest `Task_*` in `Tasks.c` → copy it → rename it → add a `{id, fn, ownStart}` row to `Table[]` in main → set `#define TASK`.
4. Change the parameters, build, flash, **Resume**, unplug, place the robot, press SW1/SW2.
5. Works → **get it validated immediately**. Doesn't work → see `08_Troubleshooting.md`.

## Questions worth asking (from the ASK lines in Tasks.c)
- Which way is "left/right": the robot's own left, or yours as you face it?
- Does it happen once, or forever/repeatedly? What should it do at the end: stop, or wait?
- "Turn 90°": a pivot in place, or a curve? Is accuracy judged by eye or measured?
- Distance measured from where: the sensor face, the front bumper, or the wheel axle?
- Should a value/LED stay latched after release, or only show while the input is held?
- LED: the RGB LED or the red LED1? What does the display show for 0, or above the range?
- Serial output: is there a required exact format? (Default to `Binary: … Decimal: … Hex: 0x..`.)
- Is a GPIO **interrupt** required for the input, or is polling OK? (The H-task example says "GPIO-based interrupt".)
- Is a library (sprintf, UART0_OutUDec) forbidden for conversions? Assume yes, and use `Num_*`.

## Submission
- Zip the **entire SC2107 folder** → upload to the Lab5 assessment page in NTULearn.
- Hand the printed checklist (manual p. 3) to the instructors before you leave.
