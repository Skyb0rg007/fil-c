#!/bin/bash

set -e
set -x

# lcms2 2.16 (replaces the copy installed from the plain lcms2-2.16.tar.gz
# during the postlc3 stage; same upstream version, now built from the
# projeny-managed Fil-C package).  utils/samples default to false and
# jpeg/tiff are auto-detected from the installed libraries.

rm -rf pizlonated-lcms2
tar -xf pizlonated-lcms2.tar.gz
cd pizlonated-lcms2
mkdir build
cd build
meson setup \
      --prefix=/usr \
      --buildtype=debugoptimized \
      ..
ninja
ninja install
cd ../..
rm -rf pizlonated-lcms2
hash -r
