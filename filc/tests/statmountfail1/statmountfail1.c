/* statmount(2) with a NULL buffer and a non-zero bufsize must be rejected by the pizlonated
   wrapper, since the kernel may write up to bufsize bytes into the buffer.  A NULL buffer with
   a non-zero size has no object to write to, so this must panic with a filc safety error. */

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

/* Local copy of struct mnt_id_req (see mountutils.h). */
struct mnt_id_req {
    uint32_t size;
    uint32_t spare;
    uint64_t mnt_id;
    uint64_t param;
    uint64_t mnt_ns_id;
};

int main(void)
{
    struct mnt_id_req req = { .size = MNT_ID_REQ_SIZE_VER1 };
    errno = 0;
    long rc = syscall(SYS_statmount, &req, NULL, 4096, 0);
    printf("statmount rc %ld errno %d\n", rc, errno);
    ZASSERT(!"Should not get here.");
    return 0;
}
