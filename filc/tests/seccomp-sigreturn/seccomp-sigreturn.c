/* A seccomp filter may make rt_sigreturn fail with an errno. Then returning from a signal handler
   comes back from the syscall in the libc's signal restorer, which used to run on into whatever
   code followed it. Now the restorer traps, so the process dies of SIGILL.

   The child denies rt_sigreturn and raises a handled signal. The runtime's own handler runs and
   returns through the restorer, so the child must die of SIGILL without getting any further. */

#define _GNU_SOURCE

#include <errno.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <signal.h>
#include <stddef.h>
#include <stdfil.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__x86_64__)
#define NR_rt_sigreturn 15
#elif defined(__aarch64__)
#define NR_rt_sigreturn 139
#else
#error "Unknown architecture"
#endif

static void handler(int signo)
{
    (void)signo;
}

int main(void)
{
    pid_t pid = fork();
    ZASSERT(pid >= 0);
    if (!pid) {
        ZASSERT(signal(SIGUSR1, handler) != SIG_ERR);
        struct sock_filter filter[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, NR_rt_sigreturn, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        };
        struct sock_fprog prog = { .len = sizeof(filter) / sizeof(filter[0]), .filter = filter };
        ZASSERT(!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
        ZASSERT(!prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog));
        raise(SIGUSR1);
        zprintf("ran on after a failed rt_sigreturn\n");
        _exit(0);
    }

    int status;
    ZASSERT(waitpid(pid, &status, 0) == pid);
    if (WIFSIGNALED(status))
        zprintf("child died of signal %d\n", WTERMSIG(status));
    else
        zprintf("child exited with %d\n", WEXITSTATUS(status));
    ZASSERT(WIFSIGNALED(status));
    ZASSERT(WTERMSIG(status) == SIGILL);
    zprintf("rt_sigreturn failure trapped\n");
    return 0;
}
