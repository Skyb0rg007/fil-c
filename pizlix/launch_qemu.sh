#!/bin/bash
#
# Copyright (c) 2025-2026 Epic Games, Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

# Runs the pizlix VM in QEMU, with sanity checks and VMware-style disk
# locking.  The options are implemented in launch_qemu_impl.sh, which we
# source below; run this script with --help to see them.
#
# This script deliberately does not trace every command with `set -x`.
# Instead, it prints English messages about what it is doing, and the QEMU
# command line is printed right before QEMU runs, so that failures can be
# debugged from the output.  Pass --debug if you really want shell tracing.

set -e

ulimit -c unlimited

# Source launch_qemu_impl.sh to get parse_args(), run_qemu(), and the option
# globals.  Sourcing it does not run QEMU, because that script only does
# anything when it is invoked directly.
# shellcheck disable=SC1090
. "$(dirname "$0")/launch_qemu_impl.sh"

parse_args "$@"

# ---------------------------------------------------------------------------
# Sanity checks.  These used to be bare `test` commands that died with no
# explanation; now they die with English messages instead.
# ---------------------------------------------------------------------------

check_file() {
    test -e "$1" || die "expected file '$1' does not exist (are you running this from the pizlix directory?)"
}

check_dir() {
    test -d "$1" || die "expected directory '$1' does not exist (are you running this from the pizlix directory?)"
}

# The name of the VMware-style lock directory for a disk image.  Our other
# scripts (make_disk_image.sh, mount_disk_image.sh, unmount_disk_image.sh) use
# the same names, so that the disks cannot be edited while a VM is using them.
lock_name() {
    case "$1" in
        *.img) echo "${1%.img}.vmdk.lck" ;;
        *) echo "$1.lck" ;;
    esac
}

say "checking that we are in the pizlix directory and that everything is where it should be"
check_dir ..
check_dir ../projects
check_dir ../llvm
check_dir etc
check_file LFS-12.2-SYSV-BOOK.pdf
check_file "$DISK_IMG"
case "$DISK_IMG" in
    *.img)
        # Our scripts create this VMware-compat descriptor next to the disk
        # image; its presence tells us that the disk was made by our scripts.
        check_file "${DISK_IMG%.img}.vmdk"
        ;;
esac
test "$EUID" -eq "$(stat -c %u ..)" || die "this script must run as uid $(stat -c %u ..) (the owner of the parent directory); you are uid $EUID"

# ---------------------------------------------------------------------------
# VMware-style locking.  See lock_name() above.
# ---------------------------------------------------------------------------

LOCK_DIRS=()

cleanup() {
    set +e
    local lock
    for lock in "${LOCK_DIRS[@]}"
    do
        say "removing lock directory $lock"
        rmdir "$lock"
    done
}

trap cleanup EXIT

make_lock() {
    local disk="$1"
    local lock
    lock="$(lock_name "$disk")"
    if test -d "$lock"
    then
        die "lock directory $lock already exists; another VM (or VMware) may be using $disk right now.  If you are sure that nothing is using it, remove $lock and try again."
    fi
    say "creating lock directory $lock to protect $disk"
    mkdir "$lock"
    LOCK_DIRS+=("$lock")
}

make_lock "$DISK_IMG"

if data_disk_attached
then
    make_lock "$DATA_DISK_IMG"
fi

say "launching the VM"
run_qemu
