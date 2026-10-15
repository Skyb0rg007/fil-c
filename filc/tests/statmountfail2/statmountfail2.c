/* The request struct is only 4 bytes (Fil-C rounds the allocation up to 16), but it claims to be
   MNT_ID_REQ_SIZE_VER1 (32) bytes.  The pizlonated wrapper must catch this before the kernel
   copies 32 bytes out of a too-small allocation.  This is exactly the attack that the request
   size validation exists to prevent. */

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdfil.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_statmount
#define SYS_statmount 457
#endif
#ifndef MNT_ID_REQ_SIZE_VER1
#define MNT_ID_REQ_SIZE_VER1 32 /* sizeof second published struct */
#endif

int main(void)
{
    struct {
        uint32_t size;
    } req = { MNT_ID_REQ_SIZE_VER1 };
    static char buf[4096];
    errno = 0;
    long rc = syscall(SYS_statmount, &req, buf, sizeof(buf), 0);
    printf("statmount rc %ld errno %d\n", rc, errno);
    ZASSERT(!"Should not get here.");
    return 0;
}
