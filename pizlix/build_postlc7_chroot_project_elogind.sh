#!/bin/bash

set -e
set -x

# elogind 255.27 (meson).  Linux-PAM must already be installed (pam=enabled).
# This provides libelogind (with the sd-bus API used by libei), the
# org.freedesktop.login1 dbus policy, and /usr/lib/security/pam_elogind.so.

rm -rf pizlonated-elogind
tar -xf pizlonated-elogind.tar.gz
cd pizlonated-elogind
mkdir -v build
cd build
meson setup \
      --prefix=/usr \
      --buildtype=debugoptimized \
      -D mode=release \
      -D pam=enabled \
      -D dbus=enabled \
      -D polkit=disabled \
      -D selinux=disabled \
      -D audit=disabled \
      -D xenctrl=disabled \
      -D tests=false \
      -D man=disabled \
      -D html=disabled \
      -D install-sysconfdir=true \
      ..
ninja
ninja install
cd ../..
rm -rf pizlonated-elogind
hash -r
