#!/bin/sh
# cinit installer: build, install as /sbin/init, pick services, sync GRUB.
set -u

SELF="$0"
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-cc}
CFLAGS=${CFLAGS:--O2 -s -static}
CONF=/etc/rc.conf
RCD=/etc/rc.d
INIT=/sbin/cinit

say() { printf '\033[1;34m[cinit]\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m[cinit] error:\033[0m %s\n' "$*" >&2; exit 1; }
ask() { printf '%s [%s]: ' "$1" "$2" >&2; read -r ans; echo "${ans:-$2}"; }

[ "$(id -u)" -eq 0 ] || die "run as root (doas)"

say "cinit installer in $SELF_DIR"

# 1. build
ans=$(ask "build cinit from source?" "yes")
if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
	$CC $CFLAGS -o "$SELF_DIR/cinit" "$SELF_DIR/cinit.c" \
		|| die "build failed"
	say "built: $SELF_DIR/cinit"
fi

# 2. install binary
ans=$(ask "install binary as $INIT (and /sbin/init)?" "yes")
if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
	install -m755 "$SELF_DIR/cinit" "$INIT" || die "cannot install $INIT"
	ln -sf cinit /sbin/init
	say "installed $INIT, /sbin/init -> cinit"
fi

# 3. pick services
echo
echo "available services in $SELF_DIR/etc/rc.d:"
printf '  '
ls "$SELF_DIR/etc/rc.d" | tr '\n' ' '
echo; echo
svc=$(ask "services to enable (space-separated)?" "dbus seatd udevd")
[ -n "$svc" ] || die "no services chosen"

for s in $svc; do
	[ -f "$SELF_DIR/etc/rc.d/$s" ] || die "unknown service: $s"
done
say "will start: $svc"

# 4. write /etc/rc.conf
tmp=$(mktemp) || die "mktemp failed"
printf '#!/bin/sh\nSERVICES="%s"\nfor svc in $SERVICES; do\n\t/etc/rc.d/$svc start\ndone\n' "$svc" > "$tmp"
install -m755 "$tmp" "$CONF"
rm -f "$tmp"
say "wrote $CONF: SERVICES=\"$svc\""

# 5. sync rc.d scripts
install -d -m755 "$RCD"
for f in "$SELF_DIR/etc/rc.d"/*; do
	install -m755 "$f" "$RCD/$(basename "$f")"
done
say "synced $RCD ($(ls "$RCD" | wc -l) scripts)"

# 6. grub
if [ -f /boot/grub/grub.cfg ] && [ -d /etc/grub.d ]; then
	ans=$(ask "update grub with init=/sbin/cinit?" "yes")
	if [ "$ans" = "yes" ] || [ "$ans" = "y" ]; then
		if ! grep -qs 'cinit' /etc/grub.d/40_custom; then
			die "add cinit entry to /etc/grub.d/40_custom manually"
		fi
		grub-mkconfig -o /boot/grub/grub.cfg \
			|| die "grub-mkconfig failed"
		say "grub.cfg regenerated"
	fi
fi

say "done. reboot with init=/sbin/cinit"
