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
#include <time.h>
#define A 32
#define L "/var/log/cinit"
#define R "/var/run/cinit"
#define C R "/ctl"
#define S R "/status"
static char v[A][64]; static pid_t p[A]; static int n, a = -1;
static char q[A][8];
static void x(int s){a = s == SIGINT;}
static void w(char *c, char *z[]){pid_t q = fork(); if (!q){execv(c, z); _exit(127);} while (waitpid(q, 0, 0) < 0 && errno == EINTR);}
static void st(void){int i; FILE*f = fopen(S, "w"); if (!f) return; for (i = 0; i < n; i++){ char b[16] = "dead"; if (p[i] > 1) snprintf(b, 16, "run(%d)", p[i]); fprintf(f, "%s %s %s\n", v[i], b, q[i]); } fclose(f);}
static void h(int r){int i; char *c[] = {"sh", "-c", "umount -a -r; swapoff -a", 0}, *m[] = {"mount", "-o", "remount,ro", "/", 0};
	for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGTERM); }
	sleep(1); for (i = 0; i < n; i++){ if (p[i] > 1) kill(-p[i], SIGKILL); }
	w("/bin/sh", c); w("/bin/mount", m); sync(); reboot(r ? RB_AUTOBOOT : RB_POWER_OFF); for (;;) pause();}
static void g(void){FILE*f = fopen("/etc/rc.conf", "r"); char l[256], *t; int i;
	if (!f) return;
	while (fgets(l, 256, f)){ if ((t = strstr(l, "SERVICES="))){ t += 9; while ((t = strtok(t, " \t\"'\n")) && n < A){ strncpy(v[n], t, 63); v[n][63] = 0; strcpy(q[n], "onfail"); n++; t = 0; } break; } }
	fclose(f);
	for (i = 0; i < n; i++){ char k[96], val[16]; FILE*ff = fopen("/etc/rc.conf", "r"); if (!ff) continue; snprintf(k, sizeof k, "%.63s_restart=", v[i]); while (fgets(l, 256, ff)){ if (strstr(l, k)){ sscanf(l + strlen(k), "%15s", val); if (!strcmp(val, "always") || !strcmp(val, "once")) strcpy(q[i], val); } } fclose(ff); }
	st();}
static void z(int i){pid_t qq = fork(); if (!qq){char c[80], lg[128]; int fd; setpgid(0, 0);
		snprintf(c, 80, "/etc/rc.d/%s", v[i]); snprintf(lg, 128, L "/%s.log", v[i]);
		fd = open(lg, O_CREAT|O_WRONLY|O_APPEND, 0644); if (fd >= 0){ dup2(fd, 1); dup2(fd, 2); close(fd); }
		execl("/bin/sh", "sh", c, "start", (char*)0); _exit(127);} p[i] = qq; st();}
static void so(int i){if (p[i] > 1){ kill(-p[i], SIGTERM); usleep(300000); if (kill(-p[i], 0) == 0) kill(-p[i], SIGKILL); p[i] = 0; st();}}
static void cc(char*b){char*cmd, *arg; int i; cmd = strtok(b, " \t\n"); arg = strtok(0, " \t\n");
	if (!cmd) return;
	if (!strcmp(cmd, "status")){ st(); return; }
	if (!strcmp(cmd, "start")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ if (p[i] <= 1) z(i); return; } return; }
	if (!strcmp(cmd, "stop")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ so(i); return; } return; }
	if (!strcmp(cmd, "restart")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ so(i); z(i); return; } return; }
	if (!strcmp(cmd, "log")){ for (i = 0; i < n; i++) if (!strcmp(v[i], arg)){ char lc[160]; snprintf(lc, sizeof lc, "tail -n 50 " L "/%.63s.log", v[i]); w("/bin/sh", (char*[]){"sh", "-c", lc, 0}); return; } return; }
	if (!strcmp(cmd, "poweroff") || !strcmp(cmd, "reboot")){ h(!strcmp(cmd, "reboot")); } }
static void ctl(void){int fd = open(C, O_RDONLY|O_NONBLOCK); char b[256]; ssize_t r;
	if (fd < 0) return;
	r = read(fd, b, sizeof(b) - 1); close(fd);
	if (r > 0){ b[r] = 0; cc(b); } }
static void setup(void){mkdir("/run", 0755); mkdir(L, 0755); mkdir(R, 0755); unlink(C); mkfifo(C, 0666); chmod(C, 0666);}
int main(void){int i, s, st2; setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin", 1); signal(SIGINT, x); signal(SIGUSR1, x); signal(SIGTERM, x);
	mount("proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0); mount("sysfs", "/sys", "sysfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0);
	mount("tmpfs", "/run", "tmpfs", MS_NOSUID|MS_NODEV, "mode=755"); mount("dev", "/dev", "devtmpfs", MS_NOSUID, "mode=755");
	mkdir("/dev/pts", 0755); mkdir("/dev/shm", 01777); mount("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC, "mode=620,gid=5");
	mount("shm", "/dev/shm", "tmpfs", MS_NOSUID|MS_NODEV, "mode=1777"); mount(0, "/", 0, MS_REMOUNT, 0); mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID|MS_NODEV, 0);
	setup(); g(); for (i = 0; i < n; i++) z(i);
	for (;;){ if (a >= 0){ h(a); a = -1; } ctl(); s = waitpid(-1, &st2, 0);
		if (s > 0){ for (i = 0; i < n; i++) if (p[i] == s){ int r0 = WIFEXITED(st2) && WEXITSTATUS(st2) == 0;
				if (r0 && !strcmp(q[i], "always")){ z(i); }
				else if (!r0 && strcmp(q[i], "once")){ sleep(1); z(i); }
				else { p[i] = 0; st(); } break; } }
		else if (errno != EINTR) sleep(1); }}