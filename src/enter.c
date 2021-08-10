/*
 * Copyright (c) 1999-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * LICENSE.QPL included in the packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/vzcalluser.h>
#include <wait.h>
#include <termios.h>
#include <pty.h>
#include <grp.h>
#include <pwd.h>
#include <err.h>
#include <sys/ioctl.h>
#include <vzctl/libvzctl.h>

#include "vzerror.h"
#include "vzctl.h"
#include "util.h"
#include "script.h"

#define DEV_TTY		"/dev/tty"

#ifndef TIOSAK
#define TIOSAK	_IO('T', 0x66)  /* "Secure Attention Key" */
#endif

static volatile sig_atomic_t child_term;
static volatile sig_atomic_t win_changed;
static volatile sig_atomic_t exit_signal;
static struct termios s_tios;

static void raw_off(void)
{
	if (tcsetattr(0, TCSADRAIN, &s_tios) == -1)
		fprintf(stderr, "Unable to restore terminal attributes: %s\n",
			strerror(errno));
}

static void raw_on(void)
{
	struct termios tios;

	if (tcgetattr(0, &tios) == -1) {
		fprintf(stderr, "Unable to get term attr: %s\n",
			strerror(errno));
		return;
	}
	/* store original settings */
	memcpy(&s_tios, &tios, sizeof(struct termios));
	cfmakeraw(&tios);
	if (tcsetattr(0, TCSADRAIN, &tios) == -1)
		fprintf(stderr, "Unable to set raw mode: %s\n",
			strerror(errno));
}

static void child_handler(int sig)
{
	child_term = 1;
}

static int tty;

static void winch(int sig)
{
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws))
		warn("Unable to get window size");
	else if (ioctl(tty, TIOCSWINSZ, &ws))
		warn("Unable to set window size");
}

static void sak(void)
{
	ioctl(tty, TIOSAK);
}

static int get_tty(struct vzctl_env_handle *h, int ntty)
{
	int tty;
	int dev;
	int ret = VZ_SYSTEM_ERROR;
	struct vzctl_ve_configure c;

	dev = open(VZCTLDEV, O_RDONLY);
	if (dev < 0) {
		fprintf(stderr, "Can't open " VZCTLDEV ": %s\n",
				strerror(errno));
		return ret;
	}

	child_term = 0;
	c.veid = vzctl2_env_get_veid(h);
	c.key = VE_CONFIGURE_OPEN_TTY;
	c.val = ntty - 1;
	c.size = 0;

	tty = ioctl(dev, VZCTL_VE_CONFIGURE, &c);
	if (tty < 0) {
		fprintf(stderr, "Error opening CT tty: %s\n", strerror(errno));
		return ret;
	}
	close(dev);
	return tty;
}

int vzcon_attach(struct vzctl_env_handle *h, int ntty)
{
	int ret;
	int status;
	int pid;
	char buf;
	const char esc = 27;
	const char enter = 13;
	int after_enter = 0;

	fprintf(stderr, "Attached to CT %s tty%d (type ESC . to detach)\n",
			vzctl2_env_get_ctid(h), ntty);

	tty = get_tty(h, ntty);

	signal(SIGCHLD, child_handler);
	signal(SIGWINCH, winch);
	winch(SIGWINCH);

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
		return VZ_RESOURCE_ERROR;
	} else if (pid == 0) {
		char bigbuf[4096];
		ssize_t nread;
		while (1) {
			if ((nread = read(tty, &bigbuf, sizeof bigbuf)) <= 0) {
				if (errno == EINTR || errno == EAGAIN)
					continue;
				if (nread < 0)
					err(1, "tty read error: %s",
					    strerror(errno));
				exit(nread < 0? 2 : 0 );
			}
			if (write(1, &bigbuf, nread) < 0) {
				err(1, "stdout write error: %s",
				    strerror(errno));
				exit(3);
			}
		}
		exit(0);
	}
	raw_on();

#define TREAD(buf)						\
	do {							\
		if (read(0, &buf, sizeof buf) <= 0) {		\
			warn("stdin read error");		\
			goto err;				\
		}						\
	} while (0)

#define TWRITE(buf)						\
	do {							\
		if (write(tty, &buf, sizeof buf) <= 0) {	\
			warn("tty write error");		\
			goto err;				\
		}						\
	} while (0)

	while (!child_term) {
		TREAD(buf);

		if (buf == esc && after_enter) {
			TREAD(buf);

			switch (buf) {
				case '.':
					if (ntty > 1)
						sak();
					goto out;
				case ',':
					goto out;
				default:
					TWRITE(esc);
					break;
			}
		}
		TWRITE(buf);
		after_enter = (buf == enter);
	}
out:
	ret = 0;
err:
	kill(pid, SIGKILL);
	while (waitpid(pid, &status, 0) == -1)
		if (errno != EINTR)
			break;
	raw_off();
	fprintf(stderr, "\nDetached from CT %s\n", vzctl2_env_get_ctid(h));

	return ret;
}
