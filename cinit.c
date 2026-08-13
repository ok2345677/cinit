#define _DEFAULT_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#define A 32
static char v[A][64]; static pid_t p[A]; static int n, a = -1;
static void x(int s){a = s == SIGINT;}
static void w(char *c, char *z[]){pid_t q = fork(); if (!q){execv(c, z); _exit(127);} while (waitpid(q, 0, 0) < 0 && errno == EINTR);}
static void h(int r){int i; char *c[] = {"sh", "-c", "umount -a -r; swapoff -a", 0}, *m[] = {"mount", "-o", "remount,ro", "/", 0};
	for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGTERM); }
	sleep(1); for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGKILL); }
	w("/bin/sh", c); w("/bin/mount", m); sync(); reboot(r ? RB_AUTOBOOT : RB_POWER_OFF); for (;;) pause();}
static void g(void){FILE *f = fopen("/etc/rc.conf", "r"); char l[256], *t; if (!f) return;
	while (fgets(l, 256, f)){ if ((t = strstr(l, "SERVICES="))){ t += 9; while ((t = strtok(t, " \t\"'\n")) && n < A){ strncpy(v[n], t, 63); v[n][63] = 0; n++; t = 0; } break; } }
	fclose(f);}
static void z(int i){pid_t q = fork(); if (!q){setpgid(0, 0); char c[80]; snprintf(c, 80, "/etc/rc.d/%s", v[i]); execl("/bin/sh", "sh", c, "start", (char*)0); _exit(127);} p[i] = q;}
int main(void){int i, s; setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin", 1); signal(SIGINT, x); signal(SIGUSR1, x); signal(SIGTERM, x);
	mount("proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0); mount("sysfs", "/sys", "sysfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0);
	mount("tmpfs", "/run", "tmpfs", MS_NOSUID|MS_NODEV, "mode=755"); mount("dev", "/dev", "devtmpfs", MS_NOSUID, "mode=755");
	mkdir("/dev/pts", 0755); mkdir("/dev/shm", 01777); mount("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC, "mode=620,gid=5");
	mount("shm", "/dev/shm", "tmpfs", MS_NOSUID|MS_NODEV, "mode=1777"); mount(0, "/", 0, MS_REMOUNT, 0); mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID|MS_NODEV, 0);
	g(); for (i = 0; i < n; i++) z(i);
	for (;;){ if (a >= 0){ h(a); a = -1; } s = waitpid(-1, 0, 0);
		if (s > 0){ for (i = 0; i < n; i++) if (p[i] == s){ if (WIFEXITED(s) && WEXITSTATUS(s) == 0) p[i] = 0; else { sleep(1); z(i); } break; } }
		else if (errno != EINTR) sleep(1); }}
