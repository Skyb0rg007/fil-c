#!/bin/sh
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

set -e
set -x

# Where the host's kernel UAPI headers live. That's /usr/include on FHS distros;
# elsewhere (e.g. NixOS) ask the host C compiler where it finds them. Set
# FILC_HOST_INCLUDE to override.
HOSTINC=$FILC_HOST_INCLUDE
if test -z "$HOSTINC"
then
    if test -d /usr/include/linux
    then
        HOSTINC=/usr/include
    else
        HOSTINC=`echo '#include <linux/version.h>' | ${CC:-cc} -E -H -x c - 2>&1 >/dev/null \
            | sed -n 's|^\. \(.*\)/linux/version\.h$|\1|p' | head -n 1`
    fi
fi
if test ! -d "$HOSTINC/linux"
then
    echo "Cannot find the kernel UAPI headers (linux/*.h); set FILC_HOST_INCLUDE." 1>&2
    exit 1
fi

rm -rf pizfix/os-include
mkdir -p pizfix/os-include
cd pizfix/os-include
ln -s $HOSTINC/linux .
arch=$(uname -m)
if test -d $HOSTINC/$(arch)-linux-gnu/asm
then
    ln -s $HOSTINC/$(arch)-linux-gnu/asm .
else
    ln -s $HOSTINC/asm .
fi
ln -s $HOSTINC/asm-generic .
