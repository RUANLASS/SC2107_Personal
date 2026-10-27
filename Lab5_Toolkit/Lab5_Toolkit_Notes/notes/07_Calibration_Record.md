# Calibration record (fill in during practice; re-check on the day)

Date / batteries: ______________________   Surface: ______________________

## IR raw ADC (TASK 900; box square to the sensor; distance measured from ______)
| Distance (cm) | rawL | rawC | rawR |
|---|---|---|---|
| 5 | | | |
| 10 | | | |
| 15 | | | |
| 20 | | | |
| 25 | | | |
| 30 | | | |
| 40 | | | |
| 50 | | | |

Curve fit d = A/(n+B)+C (if used):  L: A=____ B=____ C=____  C: A=____ B=____ C=____  R: A=____ B=____ C=____

## Toolkit.h constants
| Constant | Default | Mine | How to test |
|---|---|---|---|
| `DIST_CAL_PCT` | 100 | | TASK 903: `100 × 1000 / measured mm` |
| `TURN_CAL_PCT` | 100 | | TASK 902: `old × 360 / actual °`, then check with TASK 901 |
| `STRAIGHT_KP` | 40 | | TASK 903: raise it if the robot curves |
| `SPEED_SLOW` | 1500 | | lowest duty that still moves reliably |
| `SPEED_MED` | 2500 | | |
| `SPEED_FAST` | 4000 | | |
| `TURN_SPEED` | 2000 | | lower it if turns vary (wheel slip) |
| `LF_KP_NUM` | 6 | | TASK 904: lower if it wobbles, raise if it loses curves |
| `REF_TIME_US` | 1000 | | white reads as black → lower; black reads as white → raise |
| `MS_PER_90DEG` | 450 | | stopwatch, open-loop fallback only |
| `START_DELAY_MS` | 1000 | | |

## Measured behaviour
| Test | Result |
|---|---|
| 90° right turn error | |
| 90° left turn error | |
| 180° turn error | |
| 300 mm straight: actual distance / sideways drift | |
| 1 wheel turn by hand: tachL / tachR (expect ~360) | |
| Speed at SPEED_MED (`SpeedL_mms` / `SpeedR_mms`) | |
| Line follow max reliable speed | |
