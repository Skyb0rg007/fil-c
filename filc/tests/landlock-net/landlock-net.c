/* Tests the pizlonated Landlock wrappers, reached via syscall(2): the ABI version query, a
   ruleset handling both filesystem and network access, LANDLOCK_RULE_PATH_BENEATH and
   LANDLOCK_RULE_NET_PORT rules, and landlock_restrict_self(). Skips if the kernel doesn't
   have Landlock, or has a Landlock without network rules (ABI < 4). */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdfil.h>
#include <stdint.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

/* Landlock's UAPI, defined locally like the statmount tests do, so that this test works with any
   kernel headers (<linux/landlock.h> only exists since 5.13). */
#ifndef SYS_landlock_create_ruleset
#define SYS_landlock_create_ruleset 444
#endif
#ifndef SYS_landlock_add_rule
#define SYS_landlock_add_rule 445
#endif
#ifndef SYS_landlock_restrict_self
#define SYS_landlock_restrict_self 446
#endif

#define LANDLOCK_CREATE_RULESET_VERSION (1U << 0)
#define LANDLOCK_CREATE_RULESET_ERRATA (1U << 1)
#define LANDLOCK_RULE_PATH_BENEATH 1
#define LANDLOCK_RULE_NET_PORT 2
#define LANDLOCK_ACCESS_FS_READ_DIR (1ULL << 3)
#define LANDLOCK_ACCESS_NET_BIND_TCP (1ULL << 0)
#define LANDLOCK_ACCESS_NET_CONNECT_TCP (1ULL << 1)

struct landlock_ruleset_attr {
    uint64_t handled_access_fs;
    uint64_t handled_access_net;
    uint64_t scoped;
};

struct landlock_path_beneath_attr {
    uint64_t allowed_access;
    int32_t parent_fd;
} __attribute__((packed));

struct landlock_net_port_attr {
    uint64_t allowed_access;
    uint64_t port;
};

static struct sockaddr_in loopback(unsigned short port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return addr;
}

/* Binds a TCP socket on the loopback, returning the fd or -1 with errno set. */
static int bind_tcp(unsigned short port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    ZASSERT(fd >= 0);
    struct sockaddr_in addr = loopback(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        int the_errno = errno;
        close(fd);
        errno = the_errno;
        return -1;
    }
    return fd;
}

static unsigned short port_of(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ZASSERT(!getsockname(fd, (struct sockaddr*)&addr, &len));
    return ntohs(addr.sin_port);
}

/* Picks a port that's free right now. */
static unsigned short free_port(void)
{
    int fd = bind_tcp(0);
    ZASSERT(fd >= 0);
    unsigned short port = port_of(fd);
    close(fd);
    return port;
}

static int connect_tcp(unsigned short port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    ZASSERT(fd >= 0);
    struct sockaddr_in addr = loopback(port);
    int rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    int the_errno = errno;
    close(fd);
    errno = the_errno;
    return rc;
}

static void add_net_rule(int ruleset_fd, unsigned long long access, unsigned short port)
{
    struct landlock_net_port_attr attr = { .allowed_access = access, .port = port };
    errno = 0;
    long rc = syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_NET_PORT, &attr, 0);
    zprintf("add net rule rc %ld errno %d\n", rc, errno);
    ZASSERT(!rc);
}

int main(void)
{
    errno = 0;
    long abi = syscall(SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 0 && (errno == ENOSYS || errno == EOPNOTSUPP)) {
        zprintf("landlock not supported, skipping\n");
        zprintf("landlock net ok\n");
        return 0;
    }
    ZASSERT(abi >= 1);
    if (abi < 4) {
        zprintf("landlock ABI %ld has no network rules, skipping\n", abi);
        zprintf("landlock net ok\n");
        return 0;
    }
    if (abi >= 7) {
        errno = 0;
        long errata = syscall(SYS_landlock_create_ruleset, NULL, 0,
                              LANDLOCK_CREATE_RULESET_ERRATA);
        ZASSERT(errata >= 0);
    }

    unsigned short bind_allowed = free_port();
    unsigned short bind_denied = free_port();
    unsigned short connect_denied = free_port();
    int listener = bind_tcp(0);
    ZASSERT(listener >= 0);
    ZASSERT(!listen(listener, 1));
    unsigned short connect_allowed = port_of(listener);

    struct landlock_ruleset_attr ruleset_attr;
    memset(&ruleset_attr, 0, sizeof(ruleset_attr));
    ruleset_attr.handled_access_fs = LANDLOCK_ACCESS_FS_READ_DIR;
    ruleset_attr.handled_access_net = LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP;
    errno = 0;
    int ruleset_fd = syscall(SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);
    zprintf("create ruleset fd %d errno %d\n", ruleset_fd, errno);
    ZASSERT(ruleset_fd >= 0);

    struct landlock_path_beneath_attr path_attr = {
        .allowed_access = LANDLOCK_ACCESS_FS_READ_DIR,
        .parent_fd = open("/proc", O_PATH | O_CLOEXEC),
    };
    ZASSERT(path_attr.parent_fd >= 0);
    ZASSERT(!syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &path_attr, 0));
    close(path_attr.parent_fd);

    add_net_rule(ruleset_fd, LANDLOCK_ACCESS_NET_BIND_TCP, bind_allowed);
    add_net_rule(ruleset_fd, LANDLOCK_ACCESS_NET_CONNECT_TCP, connect_allowed);

    /* Unknown rule types are refused without panicking. */
    errno = 0;
    ZASSERT(syscall(SYS_landlock_add_rule, ruleset_fd, 1000, &path_attr, 0) == -1);
    ZASSERT(errno);

    ZASSERT(!prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0));
    errno = 0;
    long rc = syscall(SYS_landlock_restrict_self, ruleset_fd, 0);
    zprintf("restrict self rc %ld errno %d\n", rc, errno);
    ZASSERT(!rc);
    close(ruleset_fd);

    int fd = bind_tcp(bind_allowed);
    ZASSERT(fd >= 0);
    close(fd);
    ZASSERT(bind_tcp(bind_denied) == -1 && errno == EACCES);

    ZASSERT(!connect_tcp(connect_allowed));
    ZASSERT(connect_tcp(connect_denied) == -1 && errno == EACCES);
    close(listener);

    DIR* dir = opendir("/proc");
    ZASSERT(dir);
    closedir(dir);
    ZASSERT(!opendir("/") && errno == EACCES);

    zprintf("landlock net ok\n");
    return 0;
}
