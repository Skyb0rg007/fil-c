#include <stdfil.h>

int main()
{
    unsigned x = 1;
    unsigned y = 0;
    asm("" : "+r"(x), "+r"(y));
    zprintf("(1) %u\n", x / y);
    zprintf("(2) %u\n", x % y);
    x = 0x80000000;
    y = -1;
    asm("" : "+r"(x), "+r"(y));
    zprintf("(3) %u\n", x / y);
    zprintf("(4) %u\n", x % y);
    return 0;
}


