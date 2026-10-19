#!/bin/bash
#
# Copyright (c) 2025 Epic Games, Inc. All Rights Reserved.
# Copyright (c) 2026 Filip Pizlo. All Rights Reserved.
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
# THIS SOFTWARE IS PROVIDED BY FILIP PIZLO ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL FILIP PIZLO OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

# Launches the pizlix VM in QEMU.
#
# You normally want to run launch_qemu.sh instead; it does the same thing, but
# adds sanity checks and VMware-style disk locking.  This script exists so
# that QEMU can be run directly without any ceremony.
#
# Both scripts take the same options; run either one with --help to see them.
# Option precedence: the defaults below, then launch_qemu_overrides.sh (if it
# exists in the current directory), then command-line options.
#
# These scripts deliberately do not trace every command with `set -x`.
# Instead, they print English messages about what they are doing, and they
# print the exact QEMU command line just before running it, so that failures
# can be debugged from the output.  Pass --debug if you really want shell
# tracing.

set -e

ulimit -c unlimited

# ---------------------------------------------------------------------------
# Defaults.  launch_qemu_overrides.sh may override these, and command-line
# options may override both.
# ---------------------------------------------------------------------------

NCPU=`nproc`
NGIG=$(awk '/MemTotal/ {printf "%.0f\n", $2/1024/1024/2}' /proc/meminfo)
SSH_PORT=2222
ZOOM_TO_FIT=off                # used to construct the default -display option
DISPLAY_OPT=                   # if non-empty, replaces the whole -display option
DISK_IMG=disk.img              # the main (root) disk image
DATA_DISK_IMG=data-disk.img    # the second disk image
DATA_DISK_MODE=auto            # auto: attach only if it exists; explicit: attach it; no: never attach it
DEVICE_OPT="virtio-vga,xres=1024,yres=768"
EXTRA_QEMU_ARGS=()             # extra arguments to pass to QEMU

say() {
    echo "==> $*"
}

die() {
    echo "launch_qemu: error: $*" >&2
    exit 1
}

usage() {
    cat <<EOF
usage: launch_qemu.sh [options] [-- extra-qemu-arguments...]

Launches the pizlix VM in QEMU.  launch_qemu_impl.sh does the same thing but
without the sanity checks and the disk locking; launch_qemu.sh is the one you
normally want.

Options:
  -h, --help          Show this help and exit.
  --ncpu N            Number of guest CPUs (default: nproc, currently $NCPU).
  --ngig N            Guest memory in GiB (default: half of host RAM, currently $NGIG).
  --port N            Host TCP port that forwards to the guest's ssh port 22
                      (default: 2222).
  --zoom-to-fit       Use zoom-to-fit=on in the default gtk display.
  --no-zoom-to-fit    Use zoom-to-fit=off in the default gtk display (default).
  --display OPT       Override the entire -display option.  The default is
                      gtk,zoom-to-fit=off (for example: --display sdl or
                      --display none).
  --disk FILE         Main (root) disk image (default: disk.img).
  --data-disk FILE    Attach FILE as the second disk image (default name:
                      data-disk.img, which is attached only if it exists).
  --no-data-disk      Do not attach a second disk image at all.
  --device OPT        Override the entire -device option (default:
                      virtio-vga,xres=1024,yres=768).
  --qemu-arg ARG      Pass one extra argument to QEMU.  May be repeated.
  --                  Pass all remaining arguments to QEMU verbatim.
  --debug             Turn on shell tracing (set -x) for debugging this script.

Extra QEMU arguments are appended after the arguments that this script
constructs.  The defaults may also be overridden by creating
launch_qemu_overrides.sh in the current directory; it can set NCPU, NGIG,
SSH_PORT, ZOOM_TO_FIT, DISPLAY_OPT, DISK_IMG, DATA_DISK_IMG, DATA_DISK_MODE
(auto, explicit, or no), DEVICE_OPT, or EXTRA_QEMU_ARGS (a bash array).
Command-line options win over the overrides file.
EOF
}

is_natural() {
    case "$1" in
        ''|*[!0-9]*) return 1 ;;
        *) return 0 ;;
    esac
}

parse_args() {
    local arg

    if test -e launch_qemu_overrides.sh
    then
        say "sourcing launch_qemu_overrides.sh"
        . launch_qemu_overrides.sh
    fi

    while test $# -gt 0
    do
        arg="$1"
        shift
        case "$arg" in
            -h|--help)
                usage
                exit 0
                ;;
            --debug|-x)
                set -x
                ;;
            --ncpu)
                test $# -gt 0 || die "option $arg requires an argument"
                NCPU="$1"
                shift
                ;;
            --ncpu=*)
                NCPU="${arg#*=}"
                ;;
            --ngig|--gig)
                test $# -gt 0 || die "option $arg requires an argument"
                NGIG="$1"
                shift
                ;;
            --ngig=*|--gig=*)
                NGIG="${arg#*=}"
                ;;
            --port)
                test $# -gt 0 || die "option $arg requires an argument"
                SSH_PORT="$1"
                shift
                ;;
            --port=*)
                SSH_PORT="${arg#*=}"
                ;;
            --zoom-to-fit)
                ZOOM_TO_FIT=on
                ;;
            --no-zoom-to-fit)
                ZOOM_TO_FIT=off
                ;;
            --display)
                test $# -gt 0 || die "option $arg requires an argument"
                DISPLAY_OPT="$1"
                shift
                ;;
            --display=*)
                DISPLAY_OPT="${arg#*=}"
                ;;
            --disk)
                test $# -gt 0 || die "option $arg requires an argument"
                DISK_IMG="$1"
                shift
                ;;
            --disk=*)
                DISK_IMG="${arg#*=}"
                ;;
            --data-disk)
                test $# -gt 0 || die "option $arg requires an argument"
                DATA_DISK_IMG="$1"
                DATA_DISK_MODE=explicit
                shift
                ;;
            --data-disk=*)
                DATA_DISK_IMG="${arg#*=}"
                DATA_DISK_MODE=explicit
                ;;
            --no-data-disk)
                DATA_DISK_MODE=no
                ;;
            --device)
                test $# -gt 0 || die "option $arg requires an argument"
                DEVICE_OPT="$1"
                shift
                ;;
            --device=*)
                DEVICE_OPT="${arg#*=}"
                ;;
            --qemu-arg)
                test $# -gt 0 || die "option $arg requires an argument"
                EXTRA_QEMU_ARGS+=("$1")
                shift
                ;;
            --qemu-arg=*)
                EXTRA_QEMU_ARGS+=("${arg#*=}")
                ;;
            --)
                EXTRA_QEMU_ARGS+=("$@")
                break
                ;;
            *)
                die "unrecognized argument: $arg (run with --help for usage; put extra QEMU arguments after --)"
                ;;
        esac
    done

    # Validate the options and compute any derived options.

    is_natural "$NCPU" || die "NCPU must be a natural number, got: '$NCPU'"
    test "$NCPU" -ge 1 || die "NCPU must be at least 1"
    is_natural "$NGIG" || die "NGIG must be a natural number, got: '$NGIG'"
    test "$NGIG" -ge 1 || die "NGIG must be at least 1"
    is_natural "$SSH_PORT" || die "SSH_PORT must be a natural number, got: '$SSH_PORT'"
    test "$SSH_PORT" -ge 1 || die "SSH_PORT must be at least 1"
    test "$SSH_PORT" -le 65535 || die "SSH_PORT must be at most 65535"

    case "$DATA_DISK_MODE" in
        auto|explicit|no)
            ;;
        *)
            die "DATA_DISK_MODE must be auto, explicit, or no, got: '$DATA_DISK_MODE'"
            ;;
    esac

    case "$ZOOM_TO_FIT" in
        on|off)
            ;;
        *)
            die "ZOOM_TO_FIT must be on or off, got: '$ZOOM_TO_FIT'"
            ;;
    esac

    if test -z "$DISPLAY_OPT"
    then
        DISPLAY_OPT="gtk,zoom-to-fit=$ZOOM_TO_FIT"
    fi

    test -n "$DISK_IMG" || die "DISK_IMG must not be empty"
    test -n "$DISPLAY_OPT" || die "DISPLAY_OPT must not be empty (use --display none for no display)"
    test -n "$DEVICE_OPT" || die "DEVICE_OPT must not be empty"
}

# Returns success if the data disk will be attached to the VM.
data_disk_attached() {
    case "$DATA_DISK_MODE" in
        auto)
            test -e "$DATA_DISK_IMG"
            ;;
        explicit)
            test -e "$DATA_DISK_IMG" || die "data disk '$DATA_DISK_IMG' does not exist"
            ;;
        no)
            return 1
            ;;
    esac
}

run_qemu() {
    local -a args=()
    local status
    local a

    say "main disk: $DISK_IMG"
    args+=(-drive "file=$DISK_IMG,format=raw")

    if data_disk_attached
    then
        say "data disk: $DATA_DISK_IMG"
        args+=(-drive "file=$DATA_DISK_IMG,format=raw")
    else
        say "data disk: none"
    fi

    say "guest memory: ${NGIG}G, guest CPUs: $NCPU, guest ssh forwarded to localhost port $SSH_PORT"
    args+=(-m "${NGIG}G")
    args+=(-smp "$NCPU")
    args+=(-net nic)
    args+=(-net "user,hostfwd=tcp::${SSH_PORT}-:22")

    say "display: $DISPLAY_OPT, device: $DEVICE_OPT"
    args+=(-device "$DEVICE_OPT")
    args+=(-display "$DISPLAY_OPT")

    args+=(-enable-kvm)

    if test "${#EXTRA_QEMU_ARGS[@]}" -gt 0
    then
        say "extra QEMU arguments: ${EXTRA_QEMU_ARGS[*]}"
        args+=("${EXTRA_QEMU_ARGS[@]}")
    fi

    printf 'qemu-system-x86_64'
    for a in "${args[@]}"
    do
        printf ' %q' "$a"
    done
    printf '\n'

    set +e
    qemu-system-x86_64 "${args[@]}"
    status=$?
    set -e
    if test "$status" -ne 0
    then
        echo "launch_qemu: error: QEMU exited with status $status; the full command line is printed above" >&2
        exit "$status"
    fi
}

# ---------------------------------------------------------------------------
# Main.  Only runs when this script is invoked directly; launch_qemu.sh
# sources this file to reuse parse_args() and run_qemu().
# ---------------------------------------------------------------------------

if test "${BASH_SOURCE[0]}" = "$0"
then
    parse_args "$@"
    run_qemu
fi
