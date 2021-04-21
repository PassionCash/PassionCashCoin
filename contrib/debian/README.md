
Debian
====================
This directory contains files used to package passiond/passion-qt
for Debian-based Linux systems. If you compile passiond/passion-qt yourself, there are some useful files here.

## passion: URI support ##


passion-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install passion-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your passion-qt binary to `/usr/bin`
and the `../../share/pixmaps/passion128.png` to `/usr/share/pixmaps`

passion-qt.protocol (KDE)

