/* Tests the pizlonated listmount(2) wrapper, reached via syscall(SYS_listmount, ...).

   The wrapper checks that we can actually read the whole request and write num*8 bytes of
   mount IDs.  Like util-linux's mountutils.h, we define the mount API constants and structs
   locally so that this test works with any libc and any kernel headers. */

#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdfil.h>
#include <stdio.h>
#include <string.h>
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
    uint64_t list[64];
    long rc;

    /* List the mounts in our namespace, starting at the root mount. */
    errno = 0;
    rc = syscall(SYS_listmount, &req, list, 64, 0);
    /* Save the errno before doing any output.  printf(3) is allowed to clobber errno, and musl's
       printf(3) really does clobber it on the first write to a non-tty stdout, because musl does
       an ioctl(TIOCGWINSZ) to decide the buffering mode and that ioctl fails with ENOTTY. */
    int err = errno;
    printf("listmount rc %ld errno %d\n", rc, err);
    if (rc == -1 && err == ENOSYS) {
        printf("listmount not supported, skipping\n");
        printf("listmount ok\n");
        return 0;
    }
    ZASSERT(rc >= 1);
    ZASSERT(rc <= 64);
    for (long index = 0; index < rc; ++index) {
        printf("listmount id %llu\n", (unsigned long long)list[index]);
        ZASSERT(list[index]);
    }

    /* Asking for zero IDs must either succeed trivially or fail cleanly with an errno; in
       either case, it must not panic. */
    errno = 0;
    rc = syscall(SYS_listmount, &req, list, 0, 0);
    err = errno;
    printf("listmount num 0 rc %ld errno %d\n", rc, err);
    ZASSERT(rc == 0 || (rc == -1 && err != 0));

    printf("listmount ok\n");
    return 0;
}
