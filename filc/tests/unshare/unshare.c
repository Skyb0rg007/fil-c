/* unshare(2) and setns(2), through libc and through syscall(2). CLONE_FS needs no privilege, and
   setns() on a bad fd fails before the kernel checks anything else. Unsharing CLONE_NEWUSER would
   need a single-threaded process, which a Fil-C process never is, since the runtime has its own
   threads. */

#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdfil.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void)
{
    ZASSERT(!unshare(CLONE_FS));
    ZASSERT(!syscall(SYS_unshare, CLONE_FS));

    errno = 0;
    ZASSERT(unshare(CLONE_NEWUSER) == -1 && errno == EINVAL);

    errno = 0;
    ZASSERT(setns(-1, 0) == -1 && errno == EBADF);
    errno = 0;
    ZASSERT(syscall(SYS_setns, -1, 0) == -1 && errno == EBADF);

    zprintf("unshare ok\n");
    return 0;
}
