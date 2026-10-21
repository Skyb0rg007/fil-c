/* The kernel reads a whole struct landlock_net_port_attr for a LANDLOCK_RULE_NET_PORT rule, so
   handing it something smaller must panic with a filc safety error rather than let the kernel
   read out of bounds. Fil-C rounds allocations up to 16 bytes, so we pass the second half of a
   16-byte object, leaving 8 bytes in bounds. */

#define _GNU_SOURCE

#include <stdfil.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

/* Landlock's UAPI, defined locally like the statmount tests do, so that this test works with any
   kernel headers (<linux/landlock.h> only exists since 5.13). */
#ifndef SYS_landlock_add_rule
#define SYS_landlock_add_rule 445
#endif

#define LANDLOCK_RULE_NET_PORT 2
#define LANDLOCK_ACCESS_NET_BIND_TCP (1ULL << 0)

int main(void)
{
    uint64_t attr[2] = { 0, LANDLOCK_ACCESS_NET_BIND_TCP };
    long rc = syscall(SYS_landlock_add_rule, -1, LANDLOCK_RULE_NET_PORT, &attr[1], 0);
    printf("landlock_add_rule rc %ld\n", rc);
    ZASSERT(!"Should not get here.");
    return 0;
}
