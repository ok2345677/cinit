#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define M(s,t,f,fl,d) if (mount(s,t,f,fl,d)<0) fprintf(stderr,"cinit: %s: %s\n",t,strerror(errno))
static void halt(int r) { sync(); reboot(r ? RB_AUTOBOOT : RB_POWER_OFF); for (;;) pause(); }
static void onsig(int s) { halt(s == SIGINT); }
int main(void)
{
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
	M("/dev/sda1", "/boot/efi", "vfat", 0, 0);
	{ pid_t p = fork(); if (!p) { execl("/usr/bin/swapon","swapon","-a",NULL); _exit(127); } while (waitpid(p,NULL,0)<0&&errno==EINTR); }
	for (;;) {
		pid_t p = fork();
		if (!p) { execl("/bin/sh", "/bin/sh", "/etc/rc.conf", NULL); _exit(127); }
		while (waitpid(p, NULL, 0) < 0 && errno == EINTR);
		fprintf(stderr, "cinit: rc.conf exited, restarting\n");
		sleep(1);
	}
}
