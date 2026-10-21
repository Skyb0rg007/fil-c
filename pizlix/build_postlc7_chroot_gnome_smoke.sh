#!/bin/bash

set -e
set -x

# Sprint 5: non-invasive runtime smoke test for the GNOME session wiring.
# Runs INSIDE the chroot as root.  Starts the system bus and elogind long
# enough to prove they come up and talk to each other, then shuts both
# down again.  No DRM, no session, nothing persistent (all state lives in
# the tmpfs /run).

mkdir -p /run/dbus
rm -f /run/dbus/pid /run/dbus/system_bus_socket

# dbus-daemon self-daemonizes (<fork/> in the stock system.conf) and
# writes /run/dbus/pid.
dbus-daemon --system

sleep 1

# Prove the system bus answers.
dbus-send --system --dest=org.freedesktop.DBus --print-reply / \
    org.freedesktop.DBus.ListNames | head -5

# elogind sanity: the binary runs and identifies itself.
/usr/libexec/elogind --version | head -1

# Start elogind (daemonizes) and prove it claims its bus name.
/usr/libexec/elogind --daemon
sleep 1
dbus-send --system --dest=org.freedesktop.DBus --print-reply / \
    org.freedesktop.DBus.ListNames | grep -i login

# Other session pieces.
mutter --version
gnome-session --version
ls -l /usr/bin/pizlix-gnome /usr/bin/pizlix-mutter \
      /usr/share/gnome-session/sessions/pizlix-gnome.session \
      /etc/xdg/autostart/pizlix-mutter.desktop \
      /etc/xdg/autostart/pizlix-terminal.desktop

# Tear it all back down.
kill $(pidof elogind) || true
sleep 1
kill $(cat /run/dbus/pid)

# elogind mounted cgroup2 at /sys/fs/cgroup while it ran.  Take that
# nested mount down too, so the build's unmount step can unmount the
# chroot's /sys (otherwise /sys stays busy and the postlc7 snapshot tar
# walks live sysfs).  If elogind left /sys/fs/cgroup as a plain dir,
# leave it: extraction recreates the empty dir.
umount /sys/fs/cgroup 2>/dev/null || true

echo GNOME-SMOKE-OK
