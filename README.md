cinit

init for Linux. C init + POSIX service scripts +
interactive installer.

 / \ I n s t a l l /  \

```sh
git clone https://github.com/ok2345677/cinit
cd cinit
sudo sh install.sh
```

Installer asks:

1. compiler (gcc / clang)
2. build from source
3. install to /sbin/cinit (and /sbin/init symlink)
4. services to enable (space-separated, default: dbus seatd udevd)
5. regenerate grub with init=/sbin/cinit

F  i l e s 

- `cinit.c` — mounts /proc /sys /run /dev, swapon -a, loops rc.conf.
- `install.sh` — POSIX sh interactive installer.
- `etc/rc.conf` — SERVICES="hostname udevd seatd dbus network agetty chronyd crond sshd acpid".
- `etc/rc.d/*` — 21 start/stop service scripts (POSIX sh).

Services

acpid agetty avahi-daemon bluetoothd chronyd containerd crond cupsd dbus
docker hostname iwd mysqld network nginx polkitd redis seatd smartd sshd udevd

Enable at boot by listing them in SERVICES.
