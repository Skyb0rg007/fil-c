#!/bin/bash

set -e
set -x

ulimit -c unlimited

cd /sources

./build_postlc7_chroot_project_dbus.sh
hash -r
#!/bin/bash

set -e
set -x

ulimit -c unlimited

cd /sources

# PAM first: everything else in this stage (shadow, elogind) needs it.
./build_postlc7_chroot_project_pam.sh
hash -r

# shadow rebuilt with PAM + working /etc/pam.d configs.
./build_postlc7_chroot_project_shadow.sh
hash -r

# elogind (logind API + pam_elogind.so + libelogind for libei).
./build_postlc7_chroot_project_elogind.sh
hash -r

# Small support libraries.
./build_postlc7_chroot_project_dconf.sh
hash -r

./build_postlc7_chroot_project_json-glib.sh
hash -r

./build_postlc7_chroot_project_lcms2.sh
hash -r

./build_postlc7_chroot_project_libxcvt.sh
hash -r

./build_postlc7_chroot_project_libusb.sh
hash -r

./build_postlc7_chroot_project_gusb.sh
hash -r

./build_postlc7_chroot_project_colord.sh
hash -r

./build_postlc7_chroot_project_libei.sh
hash -r

# X11 extension libraries needed by mutter/GNOME.
./build_postlc7_chroot_project_x11extras.sh
hash -r

# App metadata.
./build_postlc7_chroot_project_libxmlb.sh
hash -r

./build_postlc7_chroot_project_appstream.sh
hash -r

# Build tools for libadwaita (installed under /yolo, not /usr).
./build_postlc7_chroot_project_sassc.sh
hash -r

# Everything below is Sprint 3: the GNOME Wayland session stack.
# Order: xkbcommon X11 rebuild first (mutter needs xkbcommon-x11), then
# gnome-desktop (mutter needs gnome-desktop-4), then mutter itself.

# xkbcommon rebuilt with -D enable-x11=true (installs over the postlc3
# build which had X11 disabled).
./build_postlc7_chroot_project_xkbcommon.sh
hash -r

# gnome-desktop 44.5 (libgnome-desktop-3 for gnome-session,
# libgnome-desktop-4 for mutter).
./build_postlc7_chroot_project_gnome_desktop.sh
hash -r

# mutter 46.9, the compositor.
./build_postlc7_chroot_project_mutter.sh
hash -r
