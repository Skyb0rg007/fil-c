#!/bin/bash

set -e
set -x

# gnome-text-editor 46.3 (the GNOME text editor app).
#
# Flags:
# -D enchant=enabled: spellchecking via enchant (built in an earlier stage).
#
# Dependencies from this stage: gtksourceview-5, libadwaita-1,
# editorconfig (all built earlier in this stage), plus pre-existing gtk4,
# cairo, enchant.
#
# Fil-C notes (see the projeny patch): GLIB_VERSION_MAX_ALLOWED is pinned
# to GLIB_VERSION_2_80 so that G_DEFINE_TYPE uses the pointer-based
# g_once_init_enter_pointer path required by the pizlonated glib.

rm -rf pizlonated-gnome-text-editor
tar -xf pizlonated-gnome-text-editor.tar.gz
cd pizlonated-gnome-text-editor
mkdir -v build
cd build
meson setup ..                   \
      --prefix=/usr              \
      --buildtype=debugoptimized \
      -D enchant=enabled
ninja
ninja install
cd ../..
rm -rf pizlonated-gnome-text-editor
hash -r