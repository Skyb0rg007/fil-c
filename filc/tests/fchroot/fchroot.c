/* Tests fchroot(2), which is new in Linux 7.3, via syscall(2). Skips if the kernel doesn't have
   it.

   The chroot goes into failfs (FD_FAILFS_ROOT), where every operation fails with EOPNOTSUPP.
   That doesn't need privileges, just no_new_privs and an fs_struct of our own. The runtime's
   threads share ours, so we have to unshare(CLONE_FS) first, or the kernel says EINVAL. */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdbool.h>
#include <stdfil.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef SYS_fchroot
#define SYS_fchroot 472
#endif

#define FD_FAILFS_ROOT -10004

static long sys_fchroot(int fd, unsigned flags)
{
    errno = 0;
    return syscall(SYS_fchroot, fd, flags);
}

int main(void)
{
    long rc = sys_fchroot(-1, 0);
    if (rc == -1 && errno == ENOSYS) {
        zprintf("fchroot not supported, skipping\n");
        zprintf("fchroot ok\n");
        return 0;
    }
    ZASSERT(rc == -1 && errno == EBADF);

    /* No flags are defined yet. */
    ZASSERT(sys_fchroot(FD_FAILFS_ROOT, 1) == -1 && errno == EINVAL);
    int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    ZASSERT(fd >= 0);
    ZASSERT(sys_fchroot(fd, 0) == -1 && errno == ENOTDIR);
    close(fd);

    pid_t pid = fork();
    ZASSERT(pid >= 0);
    if (!pid) {
        /* We may already have no_new_privs, for example if we run in a sandbox. */
        int no_new_privs = prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0);
        ZASSERT(no_new_privs == 0 || no_new_privs == 1);
        bool chrooted = false;
        if (!no_new_privs) {
            /* Unless we have CAP_SYS_CHROOT, we need no_new_privs. */
            if (!sys_fchroot(FD_FAILFS_ROOT, 0))
                chrooted = true;
            else if (errno == EPERM)
                ZASSERT(!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
            else
                ZASSERT(errno == EINVAL);
        }
        if (!chrooted && sys_fchroot(FD_FAILFS_ROOT, 0)) {
            ZASSERT(errno == EINVAL);
            ZASSERT(!unshare(CLONE_FS));
            ZASSERT(!sys_fchroot(FD_FAILFS_ROOT, 0));
        }
        errno = 0;
        ZASSERT(access("/", F_OK) == -1 && errno == EOPNOTSUPP);
        errno = 0;
        ZASSERT(open("/dev/null", O_RDONLY) == -1 && errno == EOPNOTSUPP);
        zprintf("root is failfs\n");
        _exit(0);
    }
    int status;
    ZASSERT(waitpid(pid, &status, 0) == pid);
    ZASSERT(WIFEXITED(status) && !WEXITSTATUS(status));

    zprintf("fchroot ok\n");
    return 0;
}
