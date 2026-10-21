#!/bin/bash

set -e
set -x

export LFS=/mnt/lfs

# Nested mounts must come down before their parents.  elogind (started by
# the postlc7 smoke test) mounts cgroup2 at $LFS/sys/fs/cgroup, which
# would otherwise keep $LFS/sys busy.  Tolerate their absence: most
# stages never create them.
umount $LFS/sys/fs/cgroup || echo whatever

umount $LFS/dev/shm || echo whatever
umount $LFS/dev/pts || echo whatever
umount $LFS/{sys,proc,run,dev} || echo whatever
