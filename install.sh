#!/bin/sh
# cinit installer: build, install as /sbin/init, pick services, sync bootloader.
# Portable: works on Alpine (syslinux), KISS (limine), Void/Gentoo/Artix (grub),
# Arch (mkinitcpio+grub), Chimera and other Linux distros.
set -u

SELF="$0"
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-}
INIT=/sbin/cinit
CONF=/etc/rc.conf
RCD=/etc/rc.d

say() { printf '\033[1;34m[cinit]\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m[cinit] error:\033[0m %s\n' "$*" >&2; exit 1; }
ask() { printf '%s [%s]: ' "$1" "$2" >&2; read -r ans; echo "${ans:-$2}"; }

[ "$(id -u)" -eq 0 ] || die "run as root (doas)"

say "cinit installer in $SELF_DIR"

# 1. pick compiler (default: $CC, then cc, then gcc, then clang)
if [ -z "$CC" ]; then
	for c in cc gcc clang; do
		if command -v "$c" >/dev/null 2>&1; then CC=$c; break; fi
	done
	[ -n "$CC" ] || die "no compiler found. install one first: alpine: apk add gcc musl-dev make; void: xbps-install gcc make; arch: pacman -S gcc make"
fi
cc=$(ask "which compiler? (found: $CC)" "$CC")
command -v "$cc" >/dev/null 2>&1 || die "compiler '$cc' not found"
say "compiler: $cc"

# 2. build (prefer static, fall back to dynamic if static libc missing)
build() { $cc $CFLAGS -o "$SELF_DIR/cinit" "$SELF_DIR/cinit.c" "$@"; }
if build -static 2>/dev/null; then
	STATIC=yes
elif build; then
	STATIC=no
	say "warning: static build failed, using dynamic (needs libc on rootfs)"
else
	die "build failed (run 'make' to see errors)"
fi
[ "$STATIC" = yes ] && say "built static: $SELF_DIR/cinit" || say "built dynamic: $SELF_DIR/cinit"

# 3. install binary
ans=$(ask "install binary as $INIT (and /sbin/init)?" "yes")
if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
	install -m755 "$SELF_DIR/cinit" "$INIT" || die "cannot install $INIT"
	ln -sf cinit /sbin/init
	say "installed $INIT, /sbin/init -> cinit"
fi

# 4. pick services
echo
echo "available services in $SELF_DIR/etc/rc.d:"
printf '  '
ls "$SELF_DIR/etc/rc.d" | tr '\n' ' '
echo; echo
svc=$(ask "services to enable (space-separated)?" "dbus seatd tty")
[ -n "$svc" ] || die "no services chosen"

for s in $svc; do
	[ -f "$SELF_DIR/etc/rc.d/$s" ] || die "unknown service: $s"
done
say "will start: $svc"

# 5. write /etc/rc.conf (cinit reads SERVICES directly)
tmp=$(mktemp) || die "mktemp failed"
printf '#!/bin/sh\nSERVICES="%s"\n' "$svc" > "$tmp"
install -m755 "$tmp" "$CONF"
rm -f "$tmp"
say "wrote $CONF: SERVICES=\"$svc\""

# 6. sync rc.d scripts
install -d -m755 "$RCD"
for f in "$SELF_DIR/etc/rc.d"/*; do
	install -m755 "$f" "$RCD/$(basename "$f")"
done
say "synced $RCD ($(ls "$RCD" | wc -l) scripts)"

# 7. bootloader: GRUB / syslinux (alpine) / limine (kiss) — auto entry
if [ -d /etc/grub.d ] && command -v grub-mkconfig >/dev/null 2>&1; then
	ans=$(ask "add init=/sbin/cinit to grub (40_custom)?" "yes")
	if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
		if ! grep -qs 'cinit' /etc/grub.d/40_custom; then
			printf 'menuentry "cinit" {\n\tlinux /boot/vmlinuz-linux root=%s init=/sbin/cinit\n}\n' \
				"$(findmnt -no SOURCE / 2>/dev/null || echo /dev/sda1)" >> /etc/grub.d/40_custom
		fi
		grub-mkconfig -o /boot/grub/grub.cfg || die "grub-mkconfig failed"
		say "grub.cfg regenerated (init=/sbin/cinit)"
	fi
elif [ -f /etc/limine.cfg ]; then
	ans=$(ask "add init=/sbin/cinit to limine?" "yes")
	if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
		if ! grep -qs 'cinit' /etc/limine.cfg; then
			printf '\n:cinit\n\tKERNEL=/boot/vmlinuz-linux\n\tCMDLINE=root=%s init=/sbin/cinit\n' \
				"$(findmnt -no SOURCE / 2>/dev/null || echo /dev/sda1)" >> /etc/limine.cfg
		fi
		limine-mkinitramfs >/dev/null 2>&1 || true
		say "limine.cfg updated (init=/sbin/cinit)"
	fi
elif [ -f /boot/extlinux/extlinux.conf ]; then
	ans=$(ask "add init=/sbin/cinit to syslinux (extlinux)?" "yes")
	if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
		sed -i "s/^TIMEOUT.*/TIMEOUT 50/" /boot/extlinux/extlinux.conf
		if ! grep -qs 'cinit' /boot/extlinux/extlinux.conf; then
			cat >> /boot/extlinux/extlinux.conf << EOF
LABEL cinit
	LINUX /vmlinuz-lts
	INITRD /initramfs-lts
	APPEND root=$(findmnt -no SOURCE / 2>/dev/null || echo /dev/sda1) init=/sbin/cinit
EOF
		fi
		say "extlinux.conf updated (init=/sbin/cinit)"
	fi
else
	say "bootloader not detected; add 'init=/sbin/cinit' to your kernel cmdline manually"
fi

say "done. reboot with init=/sbin/cinit"