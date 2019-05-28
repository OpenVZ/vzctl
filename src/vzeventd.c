/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file is part of OpenVZ. OpenVZ is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libudev.h>
#include <poll.h>

#include <vzctl/libvzctl.h>

#define VZEVENT_DIR	"/etc/vz/vzevent.d/"
#define PID_FILE	"/var/run/vzeventd.pid"
#define LOG_FILE	"/var/log/vzctl.log"

#ifndef NETLINK_VZEVENT
#define NETLINK_VZEVENT	31
#endif

static int _s_sock;
static struct udev_monitor *_s_udev_mon;
volatile sig_atomic_t _s_stopped;
static struct sockaddr_nl _s_nladd = {
	.nl_family = AF_NETLINK,
	.nl_groups = 0x01,
};

static void term_handler(int signo)
{
	_s_stopped = 1;
}

static int init_uevent(void)
{
	struct udev *udev;

	udev = udev_new();
	if (udev == NULL) {
		vzctl2_log(-1, ENOENT, "udev_new()");
		return 1;
	}

	_s_udev_mon = udev_monitor_new_from_netlink(udev, "kernel");
	if (_s_udev_mon == NULL) {
		vzctl2_log(-1, ENOENT, "udev_monitor_new_from_netlink");
		return 1;
	}

	udev_monitor_enable_receiving(_s_udev_mon);

	return 0;
}

static int init_vzevent(void)
{
	_s_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_VZEVENT);
	if (_s_sock < 0)
		return 1;

	if (bind(_s_sock, (struct sockaddr *)&_s_nladd, sizeof(_s_nladd)) == -1) {
		if (errno == ENOENT)
			vzctl2_log(-1, 0, "No vzevent support");
		else
			vzctl2_log(-1, errno, "Error in bind()");
		close(_s_sock);
		return 1;
	}

	return 0;
}

static int init(int demonize, int verbose)
{
	int rc;

	vzctl2_init_log("vzeventd");
	vzctl2_set_log_file(LOG_FILE);
	vzctl2_set_log_enable(1);
	vzctl2_set_log_level(verbose);
	vzctl2_set_log_verbose(verbose);
	vzctl2_set_log_quiet(!demonize);

	rc = init_vzevent();
	if (rc)
		return rc;

	rc = init_uevent();
	if (rc)
		return rc;

	return 0;
}

static void run_event_script(const char *event, const char *id)
{
	int pid;
	char fname[PATH_MAX];
	struct stat st;
	char idstr[512];
	char *arg[] = {fname, NULL};
	char *env[] = {"HOME=/", "TERM=linux",
		"PATH=/bin:/sbin:/usr/bin:/usr/sbin:", idstr, NULL};

	vzctl2_log(2, 0, "EVENT: %s %s", event, id);
	snprintf(idstr, sizeof(idstr), "ID=%s", id);

	snprintf(fname, sizeof(fname), VZEVENT_DIR"%s", event);
	if (stat(fname, &st)) {
		if (errno != ENOENT)
			vzctl2_log(-1, errno, "Failed to stat %s", fname);
		return;
	}

	pid = fork();
	if (pid < 0) {
		vzctl2_log(-1, errno, "Failed to fork");
	} else if (pid == 0) {
		vzctl2_log(0, 0, "Run: %s id=%s", fname, id);
		struct sigaction act = {};

		act.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &act, NULL);

		execve(arg[0], arg, env);
		vzctl2_log(-1, errno, "Failed to exec %s", arg[0]);
		exit(1);
	}
}

static int process_vzevent(void)
{
	int len;
	char buf[512];
	char event[32];
	char id[37];
	struct iovec iov = {
		.iov_base = buf,
		.iov_len = sizeof(buf)-1,
	};
	struct msghdr msg;

	bzero(&msg, sizeof(msg));
	msg.msg_name = (void *)&_s_nladd;
	msg.msg_namelen = sizeof(_s_nladd);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	len = recvmsg(_s_sock, &msg, 0);
	if (len < 0) {
		vzctl2_log(-1, errno, "Failed to receive the event message");
		/* ignore ENOBUFS error */
		if (errno == ENOBUFS)
			return 0;
		return 1;
	} else if (len == 0)
		return 0;

	buf[len] = '\0';
	/* event@id */
	if (sscanf(buf, "%31[^@]@%36s", event, id) != 2) {
		vzctl2_log(-1, 0, "Unknown event format: %s", buf);
		return 1;
	}

	run_event_script(event, id);

	return 0;
}

static void process_uevent(void)
{
	struct udev_device *udev;
	struct udev_list_entry *e;
	int fserror = 0;
	const char *n;
	const char *v;
	const char *dev = NULL;

	udev = udev_monitor_receive_device(_s_udev_mon);
	if (udev == NULL)
		return;

	udev_list_entry_foreach(e, udev_device_get_properties_list_entry(udev)) {
		n = udev_list_entry_get_name(e);
		v = udev_list_entry_get_value(e);
		if (!strcmp(n, "FS_ACTION") && 
				(!strcmp(v, "ABORT") || !strcmp(v, "ERROR")))
			fserror = 1;

		if (!strcmp(n, "FS_NAME"))
			dev = v;
	}
	if (fserror && dev != NULL)
		run_event_script("ve-fserror", dev);

	udev_device_unref(udev);
}

#define UEVENT_FD	0
#define VZEVENT_FD	1

static int mainloop(void)
{
	struct sigaction act;
	struct pollfd fds[2] = {};
	int n;

	sigemptyset(&act.sa_mask);
	act.sa_handler = term_handler;
	act.sa_flags = 0;
	sigaction(SIGTERM, &act, NULL);
	act.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &act, NULL);

	vzctl2_log(0, 0, "Starting vzeventd");

	fds[UEVENT_FD].fd = udev_monitor_get_fd(_s_udev_mon);
	fds[UEVENT_FD].events = POLLIN;
	fds[VZEVENT_FD].fd = _s_sock;
	fds[VZEVENT_FD].events = POLLIN;

	while (!_s_stopped) {
		n = poll(fds, 2, 50000);
		if (n == -1) {
			if (errno == EINTR)
				continue;
			vzctl2_log(-1, errno, "poll()");
			break;
		} else if (n == 0)
			continue;

		if (fds[UEVENT_FD].revents & POLLIN) {
			process_uevent();
		}

		if (fds[VZEVENT_FD].revents & POLLIN) {
			process_vzevent();
		}
	}

	vzctl2_log(0, 0, "Exited");
	return 0;
}

static int lock(void)
{
	int fd;
	char pid[12];

/* FIXME */
#define VZCTL_LOCK_EX   0x1
#define VZCTL_LOCK_SH   0x2
#define VZCTL_LOCK_NB   0x4
	fd = vzctl2_lock(PID_FILE, VZCTL_LOCK_EX|VZCTL_LOCK_NB, 0);
	if (fd < 0) {
		if (fd == -2)
			vzctl2_log(-1, 0, "vzevent already running");
		return 1;
	}
	sprintf(pid, "%d", getpid());
	if (pwrite(fd, pid, strlen(pid) + 1, 0) == -1)
		vzctl2_log(-1, errno, "Failed to write "PID_FILE);

	return 0;
}

static void usage(void)
{
        printf("Usage: vzeventd [-d]\n");
}

int main(int argc, char **argv)
{
	int ret, opt;
	int verbose = 1;
	int daemonize = 1;

	while ((opt = getopt(argc, argv, "dvh")) != -1) {
		switch (opt) {
			case 'd':
				daemonize = 0;
				break;
			case 'h':
				usage();
				return 0;
			case 'v':
				verbose++;
				break;
			default:
				usage();
				return 1;
		}
	}


	ret = init(daemonize, verbose);
	if (ret)
		return ret;

	if (daemonize && daemon(0, 0) < 0) {
		vzctl2_log(-1, errno, "Error in daemon()");
		return 1;
	}

	if (lock())
		return 1;

	ret = mainloop();

	unlink(PID_FILE);

	return ret;
}
