/* Realistic shape of the bug: the classic "bit reader with n == 0" mistake.  peek_bits(c, 0) shifts by 32. */
#include <stdio.h>
#include <stdlib.h>
#define G(n) __attribute__((noinline)) int op##n(int v) { printf("op" #n " %d\n", v); return n; }
G(0) G(1) G(2) G(3) G(4) G(5) G(6) G(7)
static inline unsigned peek_bits(unsigned cache, unsigned n) { return cache >> (32 - n); }
__attribute__((noinline)) int decode(unsigned cache, unsigned nbits, int v) {
    if (nbits > 3) return -1;                 /* "validated" length field from the input */
    switch (peek_bits(cache, nbits)) {        /* nbits == 0  ->  cache >> 32 */
    case 0: return op0(v);
    case 1: return op1(v);
    case 2: return op2(v);
    case 3: return op3(v);
    case 4: return op4(v);
    case 5: return op5(v);
    case 6: return op6(v);
    case 7: return op7(v);
    }
    return -2;
}

int main()
{
    decode(0x12345678, 3, 3);
    decode(0x12345678, 0, 3);
    return 0;
}

