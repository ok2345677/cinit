cinit

init for Linux. C init + POSIX service scripts +
interactive installer. Portable across Alpine, Chimera,
KISS, Void, Gentoo, and other Linux distros (glibc or musl,
gcc or clang). No dependencies beyond libc + kernel headers.

 / \ I n s t a l l /  \

```sh
git clone https://github.com/ok2345677/cinit
cd cinit
sudo sh install.sh
```

Or build manually:

```sh
make                 # static build (recommended for init)
make STATIC=0        # dynamic, if distro lacks static libc
sudo make install    # installs to /usr/local/sbin/cinit
```

Installer asks:

1. compiler (gcc / clang / cc — auto-detected)
2. static or dynamic build (auto fallback if static libc missing)
3. install to /sbin/cinit (and /sbin/init symlink)
4. services to enable (space-separated, default: dbus seatd tty)
5. regenerate grub with init=/sbin/cinit

Boot with kernel cmdline `init=/sbin/cinit`.

F  i l e s 

- `cinit.c` — pure POSIX C (c89/c99 clean). Mounts /proc /sys /run /dev,
  `mount -a` from fstab, swapon -a, reads SERVICES from rc.conf,
  forks each service, restarts crashed ones (exit 0 = done, one-shot).
- `Makefile` — portable, CC=cc, STATIC=1 by default.
- `install.sh` — POSIX sh interactive installer.
- `etc/rc.conf` — SERVICES="hostname udevd dbus seatd tty network".
- `etc/rc.d/*` — 21 start/stop service scripts (POSIX sh, exec foreground).

Services

acpid agetty avahi-daemon bluetoothd chronyd containerd crond cupsd dbus
docker hostname iwd mysqld network nginx polkitd redis seatd smartd sshd
tty udevd

Enable at boot by listing them in SERVICES.
