
Debian
====================
This directory contains files used to package kydd/kyd-qt
for Debian-based Linux systems. If you compile kydd/kyd-qt yourself, there are some useful files here.

## kyd: URI support ##


kyd-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install kyd-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your kydqt binary to `/usr/bin`
and the `../../share/pixmaps/kyd128.png` to `/usr/share/pixmaps`

kyd-qt.protocol (KDE)

