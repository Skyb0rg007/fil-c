#!/bin/bash

set -e
set -x

rm -rf pizlonated-mesa
tar -xf pizlonated-mesa.tar.gz
cd pizlonated-mesa
# The tarball is packaged from projects/mesa.projeny (URL: mesa-24.1.5.tar.xz +
# Fil-C patch): version-script symbol visibility for dlopen'd dri modules,
# bochs-drm (QEMU std VGA) driver entry point + install name, XGETBV inline asm
# removal, sparse-array tag-bit removal, EGL native handle types as real
# pointers, EGLDevice handle zptrtable encoding, glsl_type name_id decoding,
# nir_src tagged-pointer removal, queryobj buffer pointer handling, and the
# older asm-disabling (meson.build, blake3) + glibc once_flag fixes.
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
