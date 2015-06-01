/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <sys/file.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <fcntl.h>

#include "vzctl.h"
#include "vzerror.h"
#include "script.h"
#include "tmplmn.h"
#include "distrconf.h"
#include "util.h"
#include "config.h"
#include "cleanup.h"
#include "config.h"

static volatile sig_atomic_t alarm_flag;

/* standard env vars to be added in any case */
static char *envp_s[] =
{
	"HOME=/",
	"TERM=linux",
	"PATH=/bin:/sbin:/usr/bin:/usr/sbin"
};

static void alarm_handler(int sig)
{
	alarm_flag = 1;
}

char *makecmdline(char **argv)
{
	char **p;
	char *str, *sp;
	int len = 0;

	if (!argv)
		return NULL;
	p = argv;
	while (*p)
		len += strlen(*p++) + 1;
	if ((str = (char *)xmalloc(len + 1)) == NULL)
		return NULL;
	p = argv;
	sp = str;
	while (*p)
		sp += sprintf(sp, "%s ", *p++);
	return str;
}

void freearg(char **arg)
{
	char **p = arg;
	if (arg == NULL)
		return;
	while (*p)
		free(*p++);
	free(arg);
	return;
}

static void _close_fds(int close_mode, int *skip_fds)
{
	int fd, max, i;
	struct stat st;

	max = sysconf(_SC_OPEN_MAX);
	if (max < 1024)
		max = 1024;
	if (close_mode & VZCTL_CLOSE_STD) {
		fd = open("/dev/null", O_RDWR);
		if (fd != -1) {
			dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
		} else {
			close(0); close(1); close(2);
		}
	}
	for (fd = 3; fd < max; fd++) {
		for (i = 0; skip_fds[i] != fd && skip_fds[i] != -1; i++);
		if (skip_fds[i] == fd) {
			if (close_mode & VZCTL_CLOSE_NOCHECK)
				continue;
			/* Only the __vzctlfd & pipes allowed to be opened */
			if (fd == vzctl_get_vzctlfd() ||
			   (fstat(fd, &st) == 0 && S_ISFIFO(st.st_mode)))
				continue;
		}
		close(fd);
	}
}


/** Close all fd.
 * @param close_mode	VZCTL_CLOSE_STD - close [0-2]
 *			VZCTL_CLOSE_NOCHECK - skip file type check
 * @param ...		list of fds are skiped, (-1 is the end mark)
*/
void close_fds(int close_mode, ...)
{
	int fd, i;
	va_list ap;
	int skip_fds[255];

	/* build aray of skiped fds */
	va_start(ap, close_mode);
	for (i = 0; i < sizeof(skip_fds) - 1; i++) {
		fd = va_arg(ap, int);
		skip_fds[i] = fd;
		if (fd == -1)
			break;
	}
	skip_fds[i] = -1;
	va_end(ap);
	_close_fds(close_mode, skip_fds);
}

static __thread char _s_logbuf[10240];
static __thread char *plogbuf;

static void initoutput(void)
{
	plogbuf = _s_logbuf;
	*plogbuf = '\0';
}

static void addoutput(char *buf, int len)
{
	char *ep = _s_logbuf + sizeof(_s_logbuf) - 1;

	if (ep > plogbuf) {
		int lenlog = _s_logbuf + sizeof(_s_logbuf) - plogbuf;
		lenlog = (len < lenlog) ? len : lenlog;
		memcpy(plogbuf, buf, lenlog);
		plogbuf += lenlog;
		*plogbuf = 0;
	}
}

static void writeoutput(int level)
{
	char buf[4096];
	char *bp, *ep, *ps;
	int old;

	if (*_s_logbuf == '\0')
		return;
	ep = buf + sizeof(buf);
	bp = buf;
	/* Disable write to stdout */
	old = vzctl_set_log_quiet(1);
	ps = _s_logbuf;
	while (*ps) {
		char *pse = ps;
		int len2, len;

		while (*pse && *pse != '\n') ++pse;
		len = pse - ps;
		len2 = (bp + len < ep) ? len : ep - bp;
		strncpy(bp, ps, len2);
		bp += len2;
		*bp = 0;
		if (*pse == '\n')
		{
			logger(level, 0, "%s", buf);
			while (*pse && *pse == '\n') ++pse;
			bp = buf;
		}
		ps = pse;
	}
	if (bp != buf)
		logger(level, 0, "%s", buf);
	vzctl_set_log_quiet(old);
}

static int env_wait(int pid)
{
	int ret, status;

	while ((ret = waitpid(pid, &status, 0)) == -1)
		if (errno != EINTR)
			break;
	if (ret == pid) {
		ret = VZ_SYSTEM_ERROR;
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
		else if (WIFSIGNALED(status)) {
			logger(-1, 0, "Received signal %d", WTERMSIG(status));
		}
	} else {
		ret = VZ_SYSTEM_ERROR;
		logger(-1, errno, "Error in waitpid()");
	}
	return ret;
}

int call_script(char *f, struct CList *arg, struct CList *env, int quiet)
{
	struct stat st;
	char **argv = NULL;
	char **envv = NULL;
	pid_t child;
	int i = 0, envc;
	struct CList *p = arg;
	struct CList *e = env;
	char progress_buf[64];
	int progress_fd = -1;
	char *sp;
	int ret, len;
	struct sigaction act;
	int out[2];
	int argc = ListSize(arg);

	/* check that script exists */
	if (stat(f, &st) == -1)
	{
		logger(-1, errno, "File %s could not be found", f);
		return VZ_NOSCRIPT;
	}
	sigemptyset(&act.sa_mask);
	act.sa_handler = SIG_DFL;
	act.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &act, NULL);
	/* Set arguments */
	argv = (char **)xmalloc((argc + 2) * sizeof(char *));

	argv[i++] = f;
	while (p)
	{
		argv[i++] = p->str;
		p = p->next;
	}
	argv[i] = 0;

	/* Set enviroment */
	envc = sizeof(envp_s) / sizeof(envp_s[0]);
	envv = (char **)xmalloc((envc + ListSize(env) + 8) * sizeof(char *));

	for (i = 0; i < envc; i++)
		envv[i] = envp_s[i];
	for (i = envc; e; i++)
	{
		envv[i] = e->str;
		e = e->next;
	}
	if ((sp = getenv("VZ_PROGRESS_FD")) != NULL) {
		if (parse_int(sp, &progress_fd) == 0) {
			snprintf(progress_buf, sizeof(progress_buf), "VZ_PROGRESS_FD=%d",
					progress_fd);
			envv[i++] = progress_buf;
		}
	}
	envv[i] = NULL;

	if (pipe(out) < 0)
		out[0] = out[1] = -1;
	sp = makecmdline(argv);
	logger(1, 0, "Running the command: %s", sp);
	free(sp);
	/* fork */
	if ((child = fork()) == 0)
	{
		int fd;

		close_fds(VZCTL_CLOSE_STD, out[1], progress_fd, -1);
		fd = open("/dev/null", O_WRONLY);
		dup2(fd, 0);
		if (!quiet)
		{
			dup2(out[1], STDOUT_FILENO);
			dup2(out[1], STDERR_FILENO);
			close(out[0]);
			close(out[1]);
		}
		else
		{
			dup2(fd, 1);
			dup2(fd, 2);
		}
		execve(f, argv, envv);
		printf("Error exec %s : %s\n", f, strerror(errno));
		exit(1);
	}
	else if(child == -1)
	{
		logger(-1, errno, "Cannot fork %s", f);
		close(out[1]);
		return VZ_RESOURCE_ERROR;
	}
	struct vzctl_cleanup_handler *h;
	h = add_cleanup_handler(cleanup_kill_process, (void *) &child);
	if (!quiet && (out[0] > 0))
	{
		char str[1024];
		close(out[1]);
		initoutput();
		while (1)
		{
			if ((len = read(out[0], str, sizeof(str) -1)) > 0)
			{
				str[len] = 0;
				printf("%s", str);
				addoutput(str, len);
			}
			else if (len == 0)
				break;
			else if (errno != EINTR)
				break;
		}
		close(out[0]);
		writeoutput(0);
	}
	ret = env_wait(child);
	del_cleanup_handler(h);
	free(argv);
	free(envv);
	return ret;
}

char *readscript(char *name, int use_f)
{
	struct stat st;
	char *buf = NULL, *p = NULL;
	int fd, len;

	if (!name)
		return NULL;
	/* Read include file first */
	if (use_f)
		buf = readscript(VE_FUNC, 0);

	if (stat(name, &st))
		return NULL;

	if (!(fd = open(name, O_RDONLY)))
	{
		logger(-1, errno, "Error opening %s", name);
		return NULL;
	}
	if (buf)
	{
		len = st.st_size + strlen(buf);
		buf = (char *)realloc(buf, len + 2);
		p = buf + strlen(buf);
	}
	else
	{
		len = st.st_size;
		buf = (char *)realloc(buf, len + 2);
		p = buf;
	}

	buf[len] = '\n';
	buf[len + 1] = 0;
	if (read(fd, p, st.st_size) < 0)
	{
		logger(-1, errno, "Error reading %s", name);
		free(buf);
		return NULL;
	}
	close(fd);
	return buf;
}

char *MakeIpEnvVar(char *name, struct CList *list)
{
	char *buf = NULL;
	char *sp, *se;
	struct CList *l;
	int r;
	char ip_str[128];
	int buflen = 0;

	if (!list)
		return NULL;
	buflen = strlen(name) + 1;

	for (l = list; l != NULL; l = l->next) {
		if (l->str != NULL)
			buflen += strlen(l->str) + 16 + 1;
	}
	buf = (char*)xmalloc(buflen + 1);
	sp = buf;
	se = buf + buflen;

	r = snprintf(buf, se - sp, "%s=", name);
	if ((r < 0) || (sp >= se))
		goto err;
	sp += r;
	for (l = list; l != NULL; l = l->next) {
		if (l->str == NULL)
			continue;
		if (get_ip_str(l, ip_str, sizeof(ip_str)))
			continue;
		r = snprintf(sp, se - sp, "%s ", ip_str);
		if ((r < 0) || (sp >= se))
			goto err;
		sp += r;
		if (l->next == NULL) {
			*sp-- = 0;
		}
	}
	return buf;
err:
	free(buf);
	return NULL;
}

char *MakeEnvVar(char *name, struct CList *list, int extended)
{
	char *buf = NULL;
	char *sp, *se;
	struct CList *l;
	int r;
	int buflen = STR_SIZE * 100;

	if (!list)
		return NULL;

	buf = (char*)xmalloc(buflen + 1);
	sp = buf;
	se = buf + buflen;

	r = snprintf(buf, se - sp, "%s=", name);
	if ((r < 0) || (sp >= se))
		goto err;
	sp += r;
	for (l = list; l != NULL; l = l->next) {
		if (l->str == NULL)
			continue;
		r = snprintf(sp, se - sp, "%s", l->str);
		if ((r < 0) || (sp >= se))
			goto err;
		sp += r;
		if (extended) {
			if (l->val1) {
				r = snprintf(sp, se - sp, ":%lu", *l->val1);
				if ((r < 0) || (sp >= se))
					goto err;
				sp += r;
			}
			if (l->val2) {
				r = snprintf(sp, se - sp, ":%lu", *l->val2);
				if ((r < 0) || (sp >= se))
					goto err;
				sp += r;
			}
		}
		if (l->next != NULL) {
			*sp++ = ' ';
			*sp = 0;
		}
	}
	return buf;
err:
	free(buf);
	return NULL;
}


#define PROC_QUOTA	"/proc/vz/vzaquota/"
#define QUOTA_U		"/aquota.user"
#define QUOTA_G		"/aquota.group"

#define INITTAB_FILE		"/etc/inittab"
#define INITTAB_VZID		"vz:"
#define INITTAB_ACTION		INITTAB_VZID "12345:once:touch " VZFIFO_FILE

#define EVENTS_DIR		"/etc/event.d/"
#define EVENTS_FILE		EVENTS_DIR "call_on_default_rc"
#define EVENTS_SCRIPT	\
	"# This task runs if default runlevel is reached\n"	\
	"start on stopped rc2\n"					\
	"start on stopped rc3\n"					\
	"start on stopped rc4\n"					\
	"start on stopped rc5\n"					\
	"exec touch " VZFIFO_FILE "\n"

#define EVENTS_DIR_UBUNTU	"/etc/init/"
#define EVENTS_FILE_UBUNTU	EVENTS_DIR_UBUNTU "call_on_default_rc.conf"
#define EVENTS_SCRIPT_UBUNTU	\
	"# tell vzctl that start was successfull\n"		\
	"#\n"								\
	"# This task causes to tell vzctl that start was successfull\n"	\
	"\n"								\
	"description	\"tell vzctl that start was successfull\"\n"	\
	"\n"								\
	"start on stopped rc RUNLEVEL=[2345]\n"				\
	"\n"								\
	"pre-start script\n"						\
	"\twhile [ $(pgrep ifup) ]; do\n"				\
	"\t\tsleep 0.5\n"						\
	"\tdone\n"							\
	"end script\n"							\
	"\n"								\
	"task\n"							\
	"\n"								\
	"exec touch " VZFIFO_FILE

#define MAX_WAIT_TIMEOUT	60 * 60

#define SYSTEMD_BIN "systemd"
#define SBIN_INIT "/sbin/init"

static int add_inittab_entry(char *entry, char *id)
{
	FILE *rfp;
	int wfd, len, err, found;
	struct stat st;
	char buf[4096];

	wfd = -1;

	if (stat(INITTAB_FILE, &st))
		return -1;

	if ((rfp = fopen(INITTAB_FILE, "r")) == NULL) {
		fprintf(stderr, "Unable to open " INITTAB_FILE " %s\n",
			strerror(errno));
		close(wfd);
		return -1;
	}

	if ((wfd = open(INITTAB_FILE ".tmp", O_WRONLY|O_TRUNC|O_CREAT,
							st.st_mode)) == -1) {
		fprintf(stderr, "Unable to open " INITTAB_FILE ".tmp %s\n",
			strerror(errno));
		return -1;
	}
	fchmod(wfd, st.st_mode);
	fchown(wfd, st.st_uid, st.st_gid);
	err = 0;
	found = 0;
	while (!feof(rfp)) {
		if (fgets(buf, sizeof(buf), rfp) == NULL) {
			if (ferror(rfp))
				err = -1;
			break;
		}
		if (!strcmp(buf, entry)) {
			found = 1;
			break;
		}
		if (id != NULL && !strncmp(buf, id, strlen(id)))
			continue;

		len = strlen(buf);
		if (write(wfd, buf, len) == -1) {
			fprintf(stderr, "Unable to write to " INITTAB_FILE" %s\n",
				strerror(errno));
			err = -1;
			break;
		}
	}
	if (!err && !found) {
		write(wfd, entry, strlen(entry));
		write(wfd, "\n", 1);
		if (rename(INITTAB_FILE ".tmp", INITTAB_FILE)) {
			fprintf(stderr, "Unable to rename " INITTAB_FILE " %s\n",
				strerror(errno));
			err = -1;
		}
	}
	close(wfd);
	fclose(rfp);
	unlink(INITTAB_FILE ".tmp");

	return err;
}

int replace_reach_runlevel_mark(void)
{
	int wfd, err, n, is_upstart = 0, is_systemd = 0;
	struct stat st;
	char buf[4096];
	char *p;

	unlink(VZFIFO_FILE);
	if (mkfifo(VZFIFO_FILE, 0644)) {
		fprintf(stderr, "Unable to create " VZFIFO_FILE " %s\n",
			strerror(errno));
		return -1;
	}
	/* Create upstart specific script */
	if (!stat(EVENTS_DIR_UBUNTU, &st)) {
		is_upstart = 1;
		wfd = open(EVENTS_FILE_UBUNTU, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		if (wfd == -1) {
			fprintf(stderr, "Unable to create " EVENTS_FILE_UBUNTU " %s\n",
				strerror(errno));
			return -1;
		}
		write(wfd, EVENTS_SCRIPT_UBUNTU, sizeof(EVENTS_SCRIPT_UBUNTU) - 1);
		close(wfd);
	} else if (!stat(EVENTS_DIR, &st)) {
		is_upstart = 1;
		wfd = open(EVENTS_FILE, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		if (wfd == -1) {
			fprintf(stderr, "Unable to create " EVENTS_FILE " %s\n",
				strerror(errno));
			return -1;
		}
		write(wfd, EVENTS_SCRIPT, sizeof(EVENTS_SCRIPT) - 1);
		close(wfd);
	}

	/* Check for systemd */
	if (!is_upstart && (n = readlink(SBIN_INIT, buf, sizeof(buf) - 1)) > 0)
	{
		buf[n] = 0;
		if ((p = strrchr(buf, '/')) == NULL)
			p = buf;
		else
			p++;
		if (strncmp(p, SYSTEMD_BIN, sizeof(SYSTEMD_BIN) - 1) == 0)
			is_systemd = 1;
	}

	if (stat(INITTAB_FILE, &st)) {
		if (is_upstart || is_systemd)
			return 0;
		fprintf(stderr, "Warning: unable to stat " INITTAB_FILE " %s\n",
			strerror(errno));
		return -1;
	}

	err = add_inittab_entry(INITTAB_ACTION, INITTAB_VZID);

	return err;
}

int wait_on_fifo(void *data)
{
	int fd, buf, ret;
	struct sigaction act, actold;

	ret = 0;
	alarm_flag = 0;
	act.sa_flags = 0;
	act.sa_handler = alarm_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGALRM, &act, &actold);

	alarm(MAX_WAIT_TIMEOUT);
	fd = open(VZFIFO_FILE, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Unable to open " VZFIFO_FILE " %s\n",
			strerror(errno));
		ret = -1;
		goto err;
	}
	if (read(fd, &buf, sizeof(buf)) == -1)
		ret = -1;
err:
	if (alarm_flag)
		ret = VZ_EXEC_TIMEOUT;
	alarm(0);
	sigaction(SIGALRM, &actold, NULL);
	unlink(VZFIFO_FILE);
	return ret;
}
