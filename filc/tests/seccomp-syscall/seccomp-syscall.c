/* Tests the pizlonated seccomp(2) wrapper, reached via syscall(SYS_seccomp, ...), and the
   filter checks it shares with prctl(PR_SET_SECCOMP).

   First we make the probes that libseccomp makes to find out what the kernel supports, which
   must answer like a kernel would rather than panic. Then we check that filters and modes the
   runtime can't live with are refused with EINVAL without installing anything. Finally we install
   filters and check that they apply to every thread in the process, including the runtime's, and
   that the runtime keeps working under them.

   Like the statmount tests, we define the newer UAPI constants locally so that this test works
   with any kernel headers. */

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SECCOMP_GET_ACTION_AVAIL
#define SECCOMP_GET_ACTION_AVAIL 2
#endif
#ifndef SECCOMP_GET_NOTIF_SIZES
#define SECCOMP_GET_NOTIF_SIZES 3
#endif
#ifndef SECCOMP_FILTER_FLAG_LOG
#define SECCOMP_FILTER_FLAG_LOG (1UL << 1)
#endif
#ifndef SECCOMP_FILTER_FLAG_NEW_LISTENER
#define SECCOMP_FILTER_FLAG_NEW_LISTENER (1UL << 3)
#endif
#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS 0x80000000U
#endif
#ifndef SECCOMP_RET_USER_NOTIF
#define SECCOMP_RET_USER_NOTIF 0x7fc00000U
#endif

static long sys_seccomp(unsigned operation, unsigned flags, void* args)
{
    errno = 0;
    return syscall(SYS_seccomp, operation, flags, args);
}

/* We may already be running under filters, for example in a container. */
static int baseline_filters;

/* Returns the number of filters the task has, or the main thread if tid is NULL. */
static int task_filters(const char* tid)
{
    char path[64];
    if (tid)
        snprintf(path, sizeof(path), "/proc/self/task/%s/status", tid);
    else
        snprintf(path, sizeof(path), "/proc/self/status");
    FILE* file = fopen(path, "r");
    ZASSERT(file);
    char line[256];
    int mode = -1;
    int filters = -1;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "Seccomp: %d", &mode);
        sscanf(line, "Seccomp_filters: %d", &filters);
    }
    fclose(file);
    ZASSERT(filters >= 0);
    ZASSERT(mode == (filters ? SECCOMP_MODE_FILTER : 0));
    return filters;
}

/* Checks that every task has the given number of filters on top of the baseline, and returns
   how many tasks there are. */
static unsigned check_all_tasks(int expected_filters)
{
    DIR* dir = opendir("/proc/self/task");
    ZASSERT(dir);
    unsigned num_tasks = 0;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.')
            continue;
        int filters = task_filters(entry->d_name);
        if (filters != baseline_filters + expected_filters) {
            char path[64];
            char comm[32] = "";
            snprintf(path, sizeof(path), "/proc/self/task/%s/comm", entry->d_name);
            FILE* file = fopen(path, "r");
            if (file) {
                fgets(comm, sizeof(comm), file);
                fclose(file);
            }
            zprintf("task %s (%s) has %d filters, expected %d\n", entry->d_name, comm, filters,
                    baseline_filters + expected_filters);
        }
        ZASSERT(filters == baseline_filters + expected_filters);
        num_tasks++;
    }
    closedir(dir);
    return num_tasks;
}

static struct sock_fprog getsid_prog(struct sock_filter* filter, unsigned the_errno)
{
    struct sock_filter result[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getsid, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | the_errno),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    memcpy(filter, result, sizeof(result));
    struct sock_fprog prog = { .len = 4, .filter = filter };
    return prog;
}

/* Filters whose action for getsid makes the syscall lie about what happened. */
static void check_refused(unsigned action)
{
    struct sock_filter filter[4];
    struct sock_fprog prog = getsid_prog(filter, 0);
    filter[2].k = action;
    ZASSERT(sys_seccomp(SECCOMP_SET_MODE_FILTER, 0, &prog) == -1 && errno == EINVAL);
    errno = 0;
    ZASSERT(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == -1 && errno == EINVAL);
}

static void install_getsid_filter(unsigned flags, unsigned the_errno)
{
    struct sock_filter filter[4];
    struct sock_fprog prog = getsid_prog(filter, the_errno);
    long rc = sys_seccomp(SECCOMP_SET_MODE_FILTER, flags, &prog);
    zprintf("install flags 0x%x rc %ld errno %d\n", flags, rc, errno);
    ZASSERT(!rc);
}

int main(void)
{
    long rc;
    unsigned action;

    baseline_filters = task_filters(NULL);

    /* libseccomp's probe for seccomp(2) itself. */
    rc = sys_seccomp(SECCOMP_SET_MODE_STRICT, 1, NULL);
    ZASSERT(rc == -1 && errno == EINVAL);

    /* libseccomp's probe for filter flags: a NULL filter gets EFAULT if the flag is known. */
    rc = sys_seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_TSYNC, NULL);
    ZASSERT(rc == -1 && errno == EFAULT);
    rc = sys_seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_LOG, NULL);
    ZASSERT(rc == -1 && errno == EFAULT);
    rc = sys_seccomp(SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_NEW_LISTENER, NULL);
    ZASSERT(rc == -1 && errno == EINVAL);
    rc = sys_seccomp(SECCOMP_SET_MODE_FILTER, 1u << 30, NULL);
    ZASSERT(rc == -1 && errno == EINVAL);
    rc = sys_seccomp(42, 0, NULL);
    ZASSERT(rc == -1 && errno == EINVAL);

    /* libseccomp's probes for actions. */
    action = SECCOMP_RET_ALLOW;
    ZASSERT(!sys_seccomp(SECCOMP_GET_ACTION_AVAIL, 0, &action));
    action = SECCOMP_RET_ERRNO;
    ZASSERT(!sys_seccomp(SECCOMP_GET_ACTION_AVAIL, 0, &action));
    action = SECCOMP_RET_KILL_PROCESS;
    ZASSERT(!sys_seccomp(SECCOMP_GET_ACTION_AVAIL, 0, &action));
    action = SECCOMP_RET_TRAP;
    rc = sys_seccomp(SECCOMP_GET_ACTION_AVAIL, 0, &action);
    ZASSERT(rc == -1 && errno == EOPNOTSUPP);
    action = SECCOMP_RET_USER_NOTIF;
    rc = sys_seccomp(SECCOMP_GET_ACTION_AVAIL, 0, &action);
    ZASSERT(rc == -1 && errno == EOPNOTSUPP);

    /* Without NEW_LISTENER, there's no user notification, which libseccomp detects with
       this. */
    char sizes[64];
    rc = sys_seccomp(SECCOMP_GET_NOTIF_SIZES, 0, sizes);
    ZASSERT(rc == -1 && errno == EINVAL);

    ZASSERT(!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));

    /* Strict mode would kill the runtime, so it's refused. If it weren't, we'd die right here. */
    rc = sys_seccomp(SECCOMP_SET_MODE_STRICT, 0, NULL);
    ZASSERT(rc == -1 && errno == EINVAL);
    errno = 0;
    ZASSERT(prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT) == -1 && errno == EINVAL);

    /* Filters that can make the runtime's syscalls lie are refused. */
    check_refused(SECCOMP_RET_TRAP);
    check_refused(SECCOMP_RET_USER_NOTIF);
    check_refused(SECCOMP_RET_ERRNO | 0);
    check_refused(0x7fe00000); /* Not an action (yet). */

    /* So is a filter that computes its action, here from the syscall's first argument. */
    struct sock_filter computed[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[0])),
        BPF_STMT(BPF_RET | BPF_A, 0),
    };
    struct sock_fprog computed_prog = { .len = 2, .filter = computed };
    ZASSERT(sys_seccomp(SECCOMP_SET_MODE_FILTER, 0, &computed_prog) == -1 && errno == EINVAL);

    /* None of that installed anything. */
    check_all_tasks(0);

    /* Make sure that the runtime has all of its threads now, so that we're checking them. */
    zlock_runtime_threads();

    /* Filters go on every thread, with or without TSYNC. That includes threads that never run
       Fil-C code, like the initial thread when the runtime runs main on another one. */
    install_getsid_filter(SECCOMP_FILTER_FLAG_TSYNC | SECCOMP_FILTER_FLAG_LOG, EPERM);
    unsigned num_tasks = check_all_tasks(1);
    zprintf("tasks after first filter: %s\n", num_tasks > 1 ? "many" : "one");

    errno = 0;
    ZASSERT(getsid(0) == -1 && errno == EPERM);

    /* Filters stack, and the later one's errno wins since its action has the same
       precedence. */
    install_getsid_filter(0, EACCES);
    ZASSERT(check_all_tasks(2) == num_tasks);
    errno = 0;
    ZASSERT(getsid(0) == -1 && errno == EACCES);

    /* The runtime still works under the filters. */
    for (unsigned index = 0; index < 1000; ++index)
        free(malloc(index * 16));
    zgc_request_and_wait();

    zprintf("seccomp syscall ok\n");
    return 0;
}
