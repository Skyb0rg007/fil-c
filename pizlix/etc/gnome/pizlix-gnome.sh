#!/bin/bash
# Start a GNOME (Wayland) session: mutter compositor + gnome-session manager.
# mutter itself is started by gnome-session as the DisplayServer-phase
# required component (see /etc/xdg/autostart/pizlix-mutter.desktop).
set -e
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=GNOME
export XDG_RUNTIME_DIR="/run/user/$(id -u)"
mkdir -m 700 -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
exec dbus-run-session -- gnome-session --session=pizlix-gnome
