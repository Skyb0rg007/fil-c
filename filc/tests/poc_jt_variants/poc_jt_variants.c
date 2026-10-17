/* Variants of the unchecked-jump-table problem, each with a classic C bug:
   ctz(0), clz(0), abs(INT_MIN).  usage: ./poc_jt_variants {ctz|ctzll|clz|abs} <value> [v] */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define G(n) __attribute__((noinline)) int g##n(int v) { printf("g" #n " %d\n", v); return n; }
G(0) G(1) G(2) G(3) G(4) G(5) G(6) G(7)
#define SW4(e) switch (e) { case 0: return g0(v); case 1: return g1(v); case 2: return g2(v); case 3: return g3(v); } return -2;
#define SW8(e) switch (e) { case 0: return g0(v); case 1: return g1(v); case 2: return g2(v); case 3: return g3(v); case 4: return g4(v); case 5: return g5(v); case 6: return g6(v); case 7: return g7(v);} return -2;
__attribute__((noinline)) int v_ctz(unsigned m, int v) { SW4(__builtin_ctz(m) >> 3) }          /* index of first non-zero byte */
__attribute__((noinline)) int v_ctzll(unsigned long m, int v) { SW8(__builtin_ctzll(m) >> 3) }
__attribute__((noinline)) int v_clz(unsigned m, int v) { SW4(__builtin_clz(m) >> 3) }
__attribute__((noinline)) int v_abs(int x, int v) { SW8(abs(x) >> 28) }
int main(int argc, char**argv) {
    v_ctz(0, 7);
    v_ctzll(0, 7);
    v_ctz(0, 7);
    v_abs(0x80000000, 7);
    return 0;
}
