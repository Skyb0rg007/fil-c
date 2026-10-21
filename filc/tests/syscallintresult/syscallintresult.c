/* syscall(2) dispatches some syscalls to runtime wrappers that return int. Their -1 must come back
   as a long -1, not as 4294967295. */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdfil.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void)
{
    errno = 0;
    long rc = syscall(SYS_close, -1);
    zprintf("close rc %ld errno %d\n", rc, errno);
    ZASSERT(rc == -1 && errno == EBADF);

    errno = 0;
    rc = syscall(SYS_renameat2, AT_FDCWD, "/nonexistent/syscallintresult", AT_FDCWD,
                 "/nonexistent/syscallintresult2", 0);
    zprintf("renameat2 rc %ld errno %d\n", rc, errno);
    ZASSERT(rc == -1 && errno == ENOENT);

    ZASSERT(syscall(SYS_getpid) == getpid());

    zprintf("syscall int result ok\n");
    return 0;
}
