#!/bin/bash

set -e
set -x

ulimit -c unlimited

test $EUID -eq 0
id -u lfs

export LFS=/mnt/lfs

test -d $LFS

export FILCSRC=..
test -d $FILCSRC/projects

test -e $LFS/sources/lfsbuildstate
lfsbuildstate=`cat $LFS/sources/lfsbuildstate`
test "x$lfsbuildstate" = "xpostlc6"

SRCDIR=$PWD

echo "postlc7-part" > $LFS/sources/lfsbuildstate

FILCOWNER=`stat -c %U $FILCSRC`
id -u $FILCOWNER
su $FILCOWNER ./build_postlc7_sub1_packaging.sh

./build_unmount.sh
./build_mount.sh

cp -v $FILCSRC/projects/*/pizlonated-*.tar.gz $LFS/sources
cp -v build_postlc7_sub2_chroot.sh $LFS/sources
cp -v build_postlc7_chroot_project_*.sh $LFS/sources
# Plain tarball needed by postlc7: the shadow PAM rebuild compares against
# it (the build itself uses pizlonated-shadow.tar.gz, which is already in
# /sources from the earlier stage that built shadow without PAM).
cp -v shadow-4.16.0.tar.xz $LFS/sources

./build_chroot_late.sh /sources/build_postlc7_sub2_chroot.sh

echo "postlc7" > $LFS/sources/lfsbuildstate

./build_unmount.sh

cd $LFS
tar -czpf $SRCDIR/lfs-postlc7.tar.gz --exclude='var/coredumps/*' .

echo Post-libc part 7 OK
