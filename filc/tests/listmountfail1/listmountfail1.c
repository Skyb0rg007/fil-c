/* listmount(2) with num = 1000 needs 1000 * 8 = 8000 bytes of mount IDs, but the list only has
   4 elements (32 bytes, rounded up).  The pizlonated wrapper must catch this before the kernel
   writes 8000 bytes into a 32-byte allocation. */

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdfil.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_listmount
#define SYS_listmount 458
#endif
#ifndef MNT_ID_REQ_SIZE_VER1
#define MNT_ID_REQ_SIZE_VER1 32 /* sizeof second published struct */
#endif
#ifndef LSMT_ROOT
#define LSMT_ROOT 0xffffffffffffffffULL /* root mount */
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
    struct mnt_id_req req = { .size = MNT_ID_REQ_SIZE_VER1, .mnt_id = LSMT_ROOT };
    uint64_t list[4];
    errno = 0;
    long rc = syscall(SYS_listmount, &req, list, 1000, 0);
    printf("listmount rc %ld errno %d\n", rc, errno);
    ZASSERT(!"Should not get here.");
    return 0;
}
