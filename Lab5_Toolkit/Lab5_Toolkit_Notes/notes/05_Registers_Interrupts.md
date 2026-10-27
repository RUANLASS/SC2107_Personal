# Registers and interrupts from scratch

Use this when a task says "use an interrupt", or you need a peripheral the toolkit doesn't wrap.
Variables shared with an ISR **must be `volatile`**.

## GPIO configuration (any port Px, mask m)
| Want | Code |
|---|---|
| Plain GPIO (not a peripheral) | `Px->SEL0 &= ~m; Px->SEL1 &= ~m;` |
| Output | `Px->DIR \|= m;` |
| Input | `Px->DIR &= ~m;` |
| Input with pull-**up** (switch to GND, active-low) | `Px->REN \|= m; Px->OUT \|= m;` |
| Input with pull-down | `Px->REN \|= m; Px->OUT &= ~m;` |
| Set / clear / toggle output | `Px->OUT \|= m;` / `Px->OUT &= ~m;` / `Px->OUT ^= m;` |
| Read input | `(Px->IN & m)`. Active-low pressed = `((Px->IN & m) == 0)` |
| Higher drive strength (LEDs) | `Px->DS \|= m;` |

Read-modify-write (`|=`, `&=`) only touches your bits. Never write `Px->OUT = ...` on a shared port (that was the Reflectance_End bug).

## Edge interrupt on a GPIO port (only Ports 1–6 have interrupts)
| Register | Meaning |
|---|---|
| `IES` | 1 = falling edge (press on active-low), 0 = rising edge (release) |
| `IFG` | flag set by the edge; **clear it in the ISR** or the ISR re-runs forever |
| `IE`  | 1 = enable the interrupt for that pin |
| `IV`  | reading it returns 2×(pin+1) of the highest-priority pending pin **and clears that flag** |

Order: configure input + pull-up → `IES` → clear `IFG` → `IE` → NVIC priority → NVIC enable → `EnableInterrupts()`.

### Template: SW1/SW2 as interrupts (PORT1, IRQ 35, nothing else uses it)
```c
volatile uint8_t SW1Count = 0, SW2Flag = 0;

void Switch_InitInt(void){
  P1->SEL0 &= ~0x12; P1->SEL1 &= ~0x12;   // P1.1, P1.4 GPIO
  P1->DIR  &= ~0x12;                      // inputs
  P1->REN  |=  0x12; P1->OUT |= 0x12;     // pull-ups
  P1->IES  |=  0x12;                      // falling edge = press
  P1->IFG  &= ~0x12;                      // clear stale flags
  P1->IE   |=  0x12;                      // arm
  NVIC->IP[35] = 2 << 5;                  // priority 2
  NVIC->ISER[1] = 1 << (35 - 32);         // enable IRQ 35
}

void PORT1_IRQHandler(void){
  uint8_t status = P1->IV;                // reads + clears the highest pending flag
  if(status == 0x04) SW1Count++;          // P1.1 -> IV = 2*(1+1) = 0x04
  if(status == 0x0A) SW2Flag = 1;         // P1.4 -> IV = 2*(4+1) = 0x0A
}
```
In the task, call `Switch_InitInt()` once, then poll `SW1Count` / `SW2Flag` in the loop. Don't also call `WaitForButton()`/`StopRequested()` for timing-critical presses: polling works alongside, but the ISR sees the press first.
Debounce: a button bounce can count twice. Ignore edges less than 50 ms apart (`if(Millis - tLast > 50){ count++; tLast = Millis; }`).

### Bumps (PORT4, IRQ 38): already done by BumpInt.c
`BumpInt_Init(&myTask)` calls `myTask(Bump_Read())` in the ISR on every press (the flag is cleared for you). Set `BumpInt_BothEdges = 1` to get releases too. The toolkit's callback only sets a flag → `Bump_TakeEvent()`.
To use your own callback instead, call `BumpInt_Init(&MyBumpTask)` again **after** `Toolkit_Init()`. Keep the callback short: set flags only, with no delays and no motor moves.

## Periodic interrupt: TimerA1 (free)
```c
// needs inc/TimerA1.c linked into the project (Add Files -> Link)
#include "../inc/TimerA1.h"
volatile uint32_t Ticks100ms = 0;
void Every100ms(void){ Ticks100ms++; P1->OUT ^= 0x01; }
// in the task:  TimerA1_Init(&Every100ms, 50000);   // 2 us x 50000 = 100 ms
// stop it:      TIMER_A1->CTL &= ~0x0030; NVIC->ICER[0] = 1 << 10;
```
Period argument = time / 2 µs (max 65535 → 131 ms). For slower rates, count ticks in the ISR.

## Periodic interrupt: SysTick (free)
```c
volatile uint32_t SysTickCount = 0;
void SysTick_Handler(void){ SysTickCount++; }   // define it yourself
void MySysTick_Init(uint32_t period){            // period in 48 MHz cycles
  SysTick->CTRL = 0;
  SysTick->LOAD = period - 1;                   // 48000 = 1 ms, max 16,777,216 (~349 ms)
  SysTick->VAL  = 0;
  SCB->SHP[11]  = 3 << 5;                       // priority 3
  SysTick->CTRL = 0x07;                         // core clock, interrupt, enable
}
```
Don't link `SysTickInts.c` if you write your own `SysTick_Handler` (duplicate symbol).

## ADC (single channel, software trigger)
IR channels are already set up by `ADC0_InitSWTriggerCh17_12_16()`. One reading of all three:
```c
uint32_t r17, r12, r16;
void ReadIRRaw(void){ ADC_In17_12_16(&r17, &r12, &r16); }   // 17 = right, 12 = centre, 16 = left (0..16383)
```
14-bit: voltage = raw × 3.3 / 16384. Bigger raw = closer obstacle.

## Timer capture (tachometer theory)
- TimerA3 counts at 12 MHz (83.3 ns per tick), 16-bit (wraps every 5.46 ms).
- Each rising edge of encoder A captures `CCR`. Period between edges = `(now - prev) & 0xFFFF` ticks.
- Wheel: 360 edges per revolution → step period T ticks → **rpm = 2,000,000 / T**; mm/s ≈ 7,330,000 / T (D = 70 mm).
- Direction = encoder B level at the A edge (B high = forward, in Tachometer.c).
- At slow speeds the period is longer than 5.46 ms, so it wraps and gives wrong numbers. Use counts per time window instead (that's what `SpeedL_mms` does).

## PWM (motors)
TimerA0 up-down, CCR0 = 7500, clock 1.5 MHz → period 2 × 7500 / 1.5 MHz = 10 ms (100 Hz).
Duty % = duty / 7500 × 100: 1500 = 20%, 2500 = 33%, 3750 = 50%, 7000 = 93%.

## Interrupt checklist when it "doesn't fire"
1. Pin set as an input with the right pull? 2. `IES` edge correct? 3. `IFG` cleared before `IE`?
4. `IE` set? 5. NVIC `ISER` bit = IRQn (ISER[1] for IRQn ≥ 32: bit = IRQn − 32)?
6. `EnableInterrupts()` called? 7. ISR name **exactly** as in the startup file (`PORT1_IRQHandler`)?
8. Flag cleared inside the ISR (or `IV` read)?
