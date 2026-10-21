#!/bin/bash

set -e
set -x

rm -rf pizlonated-mesa
tar -xf pizlonated-mesa.tar.gz
cd pizlonated-mesa
# Fil-C compatibility fixes (see fil-c-fixes/README.md): version-script symbol
# visibility for dlopen'd dri modules, bochs-drm (QEMU std VGA) driver entry
# point + install name, XGETBV inline asm removal, sparse-array tag-bit removal,
# EGL native handle types as real pointers, EGLDevice handle zptrtable encoding,
# glsl_type name_id decoding, nir_src tagged-pointer removal, queryobj buffer
# pointer handling.  Applied with patch so the exact deltas are reviewable.
patch -p1 --forward < /sources/fil-c-fixes/fil-c-compat.patch
mkdir -v build
cd build
meson setup ..                 \
      --prefix=/usr            \
      --buildtype=debugoptimized \
      -D platforms=wayland     \
      -D gallium-drivers=swrast \
      -D vulkan-drivers=       \
      -D valgrind=disabled     \
      -D libunwind=disabled    \
      -D glx=disabled
ninja
ninja install
cd ../..
rm -rf pizlonated-mesa
