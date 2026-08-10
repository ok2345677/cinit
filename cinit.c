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

#define MAXSVC 32
#define NAMLEN 64

static char svc[MAXSVC][NAMLEN];
static pid_t pid[MAXSVC];
static int nsvc;

#define M(s,t,f,fl,d) if (mount(s,t,f,fl,d)<0) fprintf(stderr,"cinit: %s: %s\n",t,strerror(errno))
static void halt(int r) { sync(); reboot(r ? RB_AUTOBOOT : RB_POWER_OFF); for (;;) pause(); }
static void onsig(int s) { halt(s == SIGINT); }

static void readconf(void)
{
	FILE *f = fopen("/etc/rc.conf", "r");
	char line[256], *p, *tok;
	if (!f) return;
	while (fgets(line, sizeof line, f)) {
		p = strstr(line, "SERVICES=");
		if (!p) continue;
		p += 9;
		if (*p == '"' || *p == '\'') p++;
		tok = strtok(p, " \t\"'\n");
		while (tok && nsvc < MAXSVC) {
			strncpy(svc[nsvc], tok, NAMLEN-1);
			svc[nsvc][NAMLEN-1] = '\0';
			nsvc++;
			tok = strtok(NULL, " \t\"'\n");
		}
		break;
	}
	fclose(f);
}

static void spawn(int i)
{
	char path[NAMLEN+12];
	pid_t p = fork();
	if (!p) {
		snprintf(path, sizeof path, "/etc/rc.d/%s", svc[i]);
		execl("/bin/sh", "sh", path, "start", (char*)0);
		_exit(127);
	}
	pid[i] = p;
}

int main(void)
{
	int i, st;
	setenv("PATH", "/sbin:/usr/sbin:/bin:/usr/bin", 1);
	signal(SIGINT, onsig), signal(SIGUSR1, onsig), signal(SIGTERM, onsig);
	M("proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0);
	M("sysfs", "/sys", "sysfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, 0);
	M("tmpfs", "/run", "tmpfs", MS_NOSUID|MS_NODEV, "mode=755");
	M("dev", "/dev", "devtmpfs", MS_NOSUID, "mode=755");
	mkdir("/dev/pts", 0755), mkdir("/dev/shm", 01777);
	M("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC, "mode=620,gid=5");
	M("shm", "/dev/shm", "tmpfs", MS_NOSUID|MS_NODEV, "mode=1777");
	M(NULL, "/", NULL, MS_REMOUNT, 0);
	M("tmpfs", "/tmp", "tmpfs", MS_NOSUID|MS_NODEV, 0);
	{ pid_t p = fork(); if (!p) { execl("/bin/sh","sh","-c","mount -a",(char*)0); _exit(127); } while (waitpid(p,NULL,0)<0&&errno==EINTR); }
	{ pid_t p = fork(); if (!p) { execlp("swapon","swapon","-a",(char*)0); _exit(127); } while (waitpid(p,NULL,0)<0&&errno==EINTR); }
	readconf();
	for (i = 0; i < nsvc; i++) spawn(i);
	for (;;) {
		pid_t p = waitpid(-1, &st, 0);
		if (p <= 0) { if (errno == EINTR) continue; sleep(1); continue; }
		for (i = 0; i < nsvc; i++) if (pid[i] == p) {
			if (WIFEXITED(st) && WEXITSTATUS(st) == 0) {
				fprintf(stderr, "cinit: %s done\n", svc[i]);
				pid[i] = 0;
			} else {
				fprintf(stderr, "cinit: %s died, restarting\n", svc[i]);
				sleep(1);
				spawn(i);
			}
			break;
		}
	}
}
