#!/bin/sh
# cinit installer: build, install as /sbin/init, pick services, sync GRUB.
# Portable: works on Alpine, Chimera, KISS, Void, Gentoo and other Linux distros.
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
fi
cc=$(ask "which compiler?" "$CC")
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

# 7. bootloader: GRUB or syslinux/limine handled via config files
if [ -d /etc/grub.d ] && command -v grub-mkconfig >/dev/null 2>&1; then
	ans=$(ask "update grub with init=/sbin/cinit?" "yes")
	if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
		if ! grep -qs 'cinit' /etc/grub.d/40_custom; then
			die "add cinit entry to /etc/grub.d/40_custom manually"
		fi
		grub-mkconfig -o /boot/grub/grub.cfg || die "grub-mkconfig failed"
		say "grub.cfg regenerated"
	fi
fi

say "done. reboot with init=/sbin/cinit"
