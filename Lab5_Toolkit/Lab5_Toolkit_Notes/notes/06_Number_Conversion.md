# Number conversion (Lab1-style, no library)

Rule of thumb: no `sprintf`, `printf("%x")`, `itoa` or `UART0_OutUDec`. Build the characters yourself with `%`, `/`, shifts and masks. The `Num_*` functions in Toolkit.c already do this. Know how they work in case you're asked.

## Algorithms
**Decimal digits (repeated division by 10):** take `n % 10` → that's the last digit; `n /= 10`; repeat until n = 0; the digits come out **backwards**, so reverse them.
```
43: 43%10=3, 43/10=4 -> 4%10=4, 4/10=0 -> digits "3","4" -> reverse -> "43"
char = '0' + digit
```
**Any base b (2, 8, 16):** the same loop with `% b` and `/ b`. For b = 2, 8 and 16 use masks and shifts instead: hex digit = `n & 0xF`, `n >>= 4`; octal digit = `n & 7`, `n >>= 3`; binary bit = `n & 1`, `n >>= 1`.
Hex character: `"0123456789ABCDEF"[digit]`, or `digit < 10 ? '0'+digit : 'A'+digit-10`.

**Binary, MSB first, fixed width (no reverse needed):**
```c
for(i = 0; i < bits; i++) buf[i] = (n & (1u << (bits-1-i))) ? '1' : '0';
```
**Octal (not in the toolkit; copy this):**
```c
uint8_t Num_ToOct(uint32_t n, char *buf){
  char tmp[12]; uint8_t i = 0, len = 0;
  do { tmp[i++] = '0' + (n & 7); n >>= 3; } while(n);
  while(i) buf[len++] = tmp[--i];
  buf[len] = 0; return len;
}
```
**String → number:** decimal `n = n*10 + (c - '0')`; hex `n = (n << 4) | value(c)`.

**Two's complement (signed n-bit):** if the top bit (bit n−1) is 1, value = raw − 2^n.
6-bit: 0x3F → 63 − 64 = −1; 0x20 → 32 − 64 = −32; range −32 … +31. In code: `Num_SignExtend(raw, 6)`.
Negate: invert all bits and add 1 (`(~x + 1) & mask`).

**BCD:** each decimal digit in its own nibble. 43 → 0x43. Encode: repeated `%10` with `<< 4` per digit. Decode: repeated `& 0xF` × 10^k.

**Gray code:** `g = b ^ (b >> 1)`. Back: `b = g; while(g >>= 1) b ^= g;`

**Bit tricks:** test bit k `(v >> k) & 1` · set `v |= 1<<k` · clear `v &= ~(1<<k)` · toggle `v ^= 1<<k` · count ones `Count_Ones(v)` · lowest set bit `v & -v` · is a power of 2 `v && !(v & (v-1))` · reverse bits `Num_ReverseBits(v, n)`.

**Rearranging Port4 bits into Bump0–5 (what Bump_Read does):**
```
P4 pins:  7 6 5 . 3 2 . 0         -> want bits 5 4 3 2 1 0
bit0 = P4.0            (in & 0x01)
bits1-2 = P4.2-3       (in & 0x0C) >> 1
bits3-5 = P4.5-7       (in & 0xE0) >> 2
then invert for positive logic: (~result) & 0x3F
```

## IR conversion (Lab1 Convert routine)
Model: **d = A / (n + B) + C** (Lab1: A = 1195172, B = −1058, C = 0, with "if n < IRMax return 800").
Fit A, B and C from (n, d) pairs with an online curve fitter, or use 2 points for a quick A and B with C = 0:
`A = d1·d2·(n1 − n2) / (d2 − d1)` and `B = A/d1 − n1`.
Table interpolation between points (n1, d1) and (n2, d2): `d = d1 + (n1 − n)(d2 − d1)/(n1 − n2)`.

## 0–63 lookup table (6 bumps, Bump0 = bit0)
Use this to check H-task output quickly. The "Bumps pressed" column lists which bump numbers to press.

| Dec | Bin (6-bit) | Hex | Oct | BCD | Signed 6-bit | Bumps pressed |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 000000 | 0x00 | 0 | 0x0 | 0 | none |
| 1 | 000001 | 0x01 | 1 | 0x1 | 1 | 0 |
| 2 | 000010 | 0x02 | 2 | 0x2 | 2 | 1 |
| 3 | 000011 | 0x03 | 3 | 0x3 | 3 | 0,1 |
| 4 | 000100 | 0x04 | 4 | 0x4 | 4 | 2 |
| 5 | 000101 | 0x05 | 5 | 0x5 | 5 | 0,2 |
| 6 | 000110 | 0x06 | 6 | 0x6 | 6 | 1,2 |
| 7 | 000111 | 0x07 | 7 | 0x7 | 7 | 0,1,2 |
| 8 | 001000 | 0x08 | 10 | 0x8 | 8 | 3 |
| 9 | 001001 | 0x09 | 11 | 0x9 | 9 | 0,3 |
| 10 | 001010 | 0x0A | 12 | 0x10 | 10 | 1,3 |
| 11 | 001011 | 0x0B | 13 | 0x11 | 11 | 0,1,3 |
| 12 | 001100 | 0x0C | 14 | 0x12 | 12 | 2,3 |
| 13 | 001101 | 0x0D | 15 | 0x13 | 13 | 0,2,3 |
| 14 | 001110 | 0x0E | 16 | 0x14 | 14 | 1,2,3 |
| 15 | 001111 | 0x0F | 17 | 0x15 | 15 | 0,1,2,3 |
| 16 | 010000 | 0x10 | 20 | 0x16 | 16 | 4 |
| 17 | 010001 | 0x11 | 21 | 0x17 | 17 | 0,4 |
| 18 | 010010 | 0x12 | 22 | 0x18 | 18 | 1,4 |
| 19 | 010011 | 0x13 | 23 | 0x19 | 19 | 0,1,4 |
| 20 | 010100 | 0x14 | 24 | 0x20 | 20 | 2,4 |
| 21 | 010101 | 0x15 | 25 | 0x21 | 21 | 0,2,4 |
| 22 | 010110 | 0x16 | 26 | 0x22 | 22 | 1,2,4 |
| 23 | 010111 | 0x17 | 27 | 0x23 | 23 | 0,1,2,4 |
| 24 | 011000 | 0x18 | 30 | 0x24 | 24 | 3,4 |
| 25 | 011001 | 0x19 | 31 | 0x25 | 25 | 0,3,4 |
| 26 | 011010 | 0x1A | 32 | 0x26 | 26 | 1,3,4 |
| 27 | 011011 | 0x1B | 33 | 0x27 | 27 | 0,1,3,4 |
| 28 | 011100 | 0x1C | 34 | 0x28 | 28 | 2,3,4 |
| 29 | 011101 | 0x1D | 35 | 0x29 | 29 | 0,2,3,4 |
| 30 | 011110 | 0x1E | 36 | 0x30 | 30 | 1,2,3,4 |
| 31 | 011111 | 0x1F | 37 | 0x31 | 31 | 0,1,2,3,4 |
| 32 | 100000 | 0x20 | 40 | 0x32 | -32 | 5 |
| 33 | 100001 | 0x21 | 41 | 0x33 | -31 | 0,5 |
| 34 | 100010 | 0x22 | 42 | 0x34 | -30 | 1,5 |
| 35 | 100011 | 0x23 | 43 | 0x35 | -29 | 0,1,5 |
| 36 | 100100 | 0x24 | 44 | 0x36 | -28 | 2,5 |
| 37 | 100101 | 0x25 | 45 | 0x37 | -27 | 0,2,5 |
| 38 | 100110 | 0x26 | 46 | 0x38 | -26 | 1,2,5 |
| 39 | 100111 | 0x27 | 47 | 0x39 | -25 | 0,1,2,5 |
| 40 | 101000 | 0x28 | 50 | 0x40 | -24 | 3,5 |
| 41 | 101001 | 0x29 | 51 | 0x41 | -23 | 0,3,5 |
| 42 | 101010 | 0x2A | 52 | 0x42 | -22 | 1,3,5 |
| 43 | 101011 | 0x2B | 53 | 0x43 | -21 | 0,1,3,5 |
| 44 | 101100 | 0x2C | 54 | 0x44 | -20 | 2,3,5 |
| 45 | 101101 | 0x2D | 55 | 0x45 | -19 | 0,2,3,5 |
| 46 | 101110 | 0x2E | 56 | 0x46 | -18 | 1,2,3,5 |
| 47 | 101111 | 0x2F | 57 | 0x47 | -17 | 0,1,2,3,5 |
| 48 | 110000 | 0x30 | 60 | 0x48 | -16 | 4,5 |
| 49 | 110001 | 0x31 | 61 | 0x49 | -15 | 0,4,5 |
| 50 | 110010 | 0x32 | 62 | 0x50 | -14 | 1,4,5 |
| 51 | 110011 | 0x33 | 63 | 0x51 | -13 | 0,1,4,5 |
| 52 | 110100 | 0x34 | 64 | 0x52 | -12 | 2,4,5 |
| 53 | 110101 | 0x35 | 65 | 0x53 | -11 | 0,2,4,5 |
| 54 | 110110 | 0x36 | 66 | 0x54 | -10 | 1,2,4,5 |
| 55 | 110111 | 0x37 | 67 | 0x55 | -9 | 0,1,2,4,5 |
| 56 | 111000 | 0x38 | 70 | 0x56 | -8 | 3,4,5 |
| 57 | 111001 | 0x39 | 71 | 0x57 | -7 | 0,3,4,5 |
| 58 | 111010 | 0x3A | 72 | 0x58 | -6 | 1,3,4,5 |
| 59 | 111011 | 0x3B | 73 | 0x59 | -5 | 0,1,3,4,5 |
| 60 | 111100 | 0x3C | 74 | 0x60 | -4 | 2,3,4,5 |
| 61 | 111101 | 0x3D | 75 | 0x61 | -3 | 0,2,3,4,5 |
| 62 | 111110 | 0x3E | 76 | 0x62 | -2 | 1,2,3,4,5 |
| 63 | 111111 | 0x3F | 77 | 0x63 | -1 | 0,1,2,3,4,5 |
