#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
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
#define LOG "/var/log/cinit"
#define RUN "/var/run/cinit"
#define CTL RUN "/ctl"
#define STF RUN "/status"
static char v[A][64]; static pid_t p[A]; static int n, a = -1;
static char pol[A][8]; /* restart policy: always|onfail|once */
static void x(int s){a = s == SIGINT;}
static void w(char *c, char *z[]){pid_t q = fork(); if (!q){execv(c, z); _exit(127);} while (waitpid(q, 0, 0) < 0 && errno == EINTR);}
static void h(int r){int i; char *c[] = {"sh", "-c", "umount -a -r; swapoff -a", 0}, *m[] = {"mount", "-o", "remount,ro", "/", 0};
	for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGTERM); }
	sleep(1); for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGKILL); }
	w("/bin/sh", c); w("/bin/mount", m); sync(); reboot(r ? RB_AUTOBOOT : RB_POWER_OFF); for (;;) pause();}
static void status(void){int i; FILE*f = fopen(STF, "w"); if (!f) return;
	for (i = 0; i < n; i++){ char st[16] = "dead"; if (p[i] > 1) snprintf(st, 16, "run(%d)", p[i]);
		fprintf(f, "%s %s %s\n", v[i], st, pol[i]); } fclose(f);}
static void g(void){FILE *f = fopen("/etc/rc.conf", "r"); char l[256], *t; int i;
	if (!f) return;
	while (fgets(l, 256, f)){ if ((t = strstr(l, "SERVICES="))){ t += 9; while ((t = strtok(t, " \t\"'\n")) && n < A){ strncpy(v[n], t, 63); v[n][63] = 0; strcpy(pol[n], "onfail"); n++; t = 0; } break; } }
	fclose(f);
	/* per-service policy: service_restart=always|onfail|once */
	for (i = 0; i < n; i++){ char key[80], val[16]; FILE *ff = fopen("/etc/rc.conf", "r"); if (!ff) continue;
		snprintf(key, 80, "%s_restart=", v[i]); while (fgets(l, 256, ff)){ if (strstr(l, key)){ sscanf(l + strlen(key), "%15s", val);
			if (!strcmp(val, "always") || !strcmp(val, "once")) strcpy(pol[i], val); } } fclose(ff); }
	status();}
static void z(int i){pid_t q = fork(); if (!q){char c[80], lg[128]; int fd; setpgid(0, 0);
		snprintf(c, 80, "/etc/rc.d/%s", v[i]); snprintf(lg, 128, LOG "/%s.log", v[i]);
		fd = open(lg, O_CREAT|O_WRONLY|O_APPEND, 0644); if (fd >= 0){ dup2(fd, 1); dup2(fd, 2); close(fd); }
		execl("/bin/sh", "sh", c, "start", (char*)0); _exit(127);} p[i] = q; status();}
static void stopone(int i){if (p[i] > 1){ kill(-p[i], SIGTERM); usleep(300000); if (kill(-p[i], 0) == 0) kill(-p[i], SIGKILL); p[i] = 0; status();}}
static void ctlcmd(char *buf){char *cmd, *arg; int i; cmd = strtok(buf, " \t\n"); arg = strtok(0, " \t\n");
	if (!cmd) return;
	if (!strcmp(cmd, "status")){ status(); return; }
	if (!strcmp(cmd, "start")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ if (p[i] <= 1) z(i); return; } return; }
	if (!strcmp(cmd, "stop")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ stopone(i); return; } return; }
	if (!strcmp(cmd, "restart")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ stopone(i); z(i); return; } return; }
	if (!strcmp(cmd, "log")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ char c[160]; snprintf(c, 160, "tail -n %ld", (long)50); w("/bin/sh", (char*[]){"sh", "-c", c, 0}); return; } return; }
	if (!strcmp(cmd, "poweroff") || !strcmp(cmd, "reboot")){ h(!strcmp(cmd, "reboot")); } }
static void ctl(void){int fd = open(CTL, O_RDONLY|O_NONBLOCK); char buf[256]; ssize_t r;
	if (fd < 0) return; r = read(fd, buf, sizeof(buf) - 1); close(fd);
	if (r > 0){ buf[r] = 0; ctlcmd(buf); } }
static void setup(void){mkdir("/run", 0755); mkdir(LOG, 0755); mkdir(RUN, 0755); unlink(CTL); mkfifo(CTL, 0666);}
int main(void){int i, s, st, ctlfd; setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin", 1); signal(SIGINT, x); signal(SIGUSR1, x); signal(SIGTERM, x);
	mount("proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0); mount("sysfs", "/sys", "sysfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0);
	mount("tmpfs", "/run", "tmpfs", MS_NOSUID|MS_NODEV, "mode=755"); mount("dev", "/dev", "devtmpfs", MS_NOSUID, "mode=755");
	mkdir("/dev/pts", 0755); mkdir("/dev/shm", 01777); mount("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC, "mode=620,gid=5");
	mount("shm", "/dev/shm", "tmpfs", MS_NOSUID|MS_NODEV, "mode=1777"); mount(0, "/", 0, MS_REMOUNT, 0); mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID|MS_NODEV, 0);
	setup(); g(); for (i = 0; i < n; i++) z(i); ctlfd = open(CTL, O_RDONLY|O_NONBLOCK); close(ctlfd);
	for (;;){ if (a >= 0){ h(a); a = -1; } ctl(); s = waitpid(-1, &st, 0);
		if (s > 0){ for (i = 0; i < n; i++) if (p[i] == s){ int r0 = WIFEXITED(st) && WEXITSTATUS(st) == 0;
				if (r0 && !strcmp(pol[i], "always")){ z(i); }
				else if (!r0 && strcmp(pol[i], "once")){ sleep(1); z(i); }
				else { p[i] = 0; status(); } break; } }
		else if (errno != EINTR) sleep(1); }}