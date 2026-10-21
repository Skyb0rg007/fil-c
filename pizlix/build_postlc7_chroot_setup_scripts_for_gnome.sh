#!/bin/bash

set -e
set -x

# Sprint 5: session wiring for the GNOME (Wayland) session.  Mirrors
# build_postlc3_chroot_setup_scripts_for_weston.sh (files live in
# /sources/etc and get copied into place, then rcN.d symlinks are made).

# Bootscripts.  dbus first (S20, before sshd's S30), then elogind (S25):
# elogind claims org.freedesktop.login1 on the system bus, and
# pam_elogind (used by login) needs elogind reachable before gettys run.
cp -v /sources/etc/init.d/dbus /etc/init.d/dbus
chmod -v 755 /etc/init.d/dbus

cp -v /sources/etc/init.d/elogind /etc/init.d/elogind
chmod -v 755 /etc/init.d/elogind

for n in 2 3 4 5; do
    ln -sfv ../init.d/dbus /etc/rc.d/rc${n}.d/S20dbus
    ln -sfv ../init.d/elogind /etc/rc.d/rc${n}.d/S25elogind
done

# K links mirror the seatd pattern (K20seatd/K25seatd... see
# build_postlc3_chroot_setup_scripts_for_weston.sh).
for n in 0 1 6; do
    ln -sfv ../init.d/dbus /etc/rc.d/rc${n}.d/K20dbus
    ln -sfv ../init.d/elogind /etc/rc.d/rc${n}.d/K25elogind
done

# The session definition.  gnome-session looks the session up in
# /usr/share/gnome-session/sessions/ and resolves RequiredComponents
# entries by desktop-file id.
mkdir -pv /usr/share/gnome-session/sessions
cp -v /sources/etc/gnome-session/pizlix-gnome.session \
      /usr/share/gnome-session/sessions/pizlix-gnome.session

# Autostart entries: mutter (DisplayServer phase) and a terminal.
mkdir -pv /etc/xdg/autostart
cp -v /sources/etc/xdg/autostart/pizlix-mutter.desktop /etc/xdg/autostart/
cp -v /sources/etc/xdg/autostart/pizlix-terminal.desktop /etc/xdg/autostart/

# The launchers: /usr/bin/pizlix-gnome (what the user runs) and
# /usr/bin/pizlix-mutter (the DisplayServer-phase wrapper).
cp -v /sources/etc/gnome/pizlix-gnome.sh /usr/bin/pizlix-gnome
chmod -v 755 /usr/bin/pizlix-gnome
cp -v /sources/etc/gnome/pizlix-mutter.sh /usr/bin/pizlix-mutter
chmod -v 755 /usr/bin/pizlix-mutter

# dbus and elogind need a machine id (BLFS dbus page: dbus-uuidgen
# --ensure plus the /etc/machine-id symlink).  It was missing so far.
dbus-uuidgen --ensure
ln -sfv /var/lib/dbus/machine-id /etc/machine-id

# Make shadow's login use PAM (USE_PAM was not set yet) so that
# pam_elogind registers every console login with elogind - the
# prerequisite for mutter to TakeControl of seat0.
grep -q '^USE_PAM' /etc/login.defs || echo 'USE_PAM yes' >> /etc/login.defs

# elogind requires /run/user to be 0755; the LFS mountvirtfs bootscript
# creates it 1777 and elogind then refuses every session
# ("Directory /run/user ... too permissive ... refusing").
sed -i 's|mkdir -m 1777 /run/user|mkdir -m 755 /run/user|' /etc/rc.d/init.d/mountvirtfs
grep -n 'run/user' /etc/rc.d/init.d/mountvirtfs

grep '^USE_PAM' /etc/login.defs
