/*
*  Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/sendfile.h>
#include <mntent.h>

#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include <limits.h>

#include "vzctl.h"
#include "util.h"
#include "list.h"
#include "util.h"
#include "vzerror.h"
#include "config.h"

#ifndef NR_OPEN
#define NR_OPEN 1024
#endif

static char *envp_bash[] = {"HOME=/", "TERM=linux",
	"PATH=/bin:/sbin:/usr/bin:/usr/sbin:.", NULL};

/* check ipv4 x.x.x.x notation */
int check_ipv4(char *ip)
{
	int cnt = 0;
	char *p = ip;

	while (*p++ != '\0')
		if (*p == '.') cnt++;

	if (cnt != 3)
		return 0;
	return 1;
}

int parse_ip(char *str, char **ipstr, unsigned int *mask)
{
	int ret;
	char *maskstr;
	unsigned int ip[4];
	char dst[65];
	int family;

	if ((maskstr = strchr(str, '/')) != NULL)
		*maskstr++ = 0;
	if ((family = get_netaddr(str, ip)) == -1)
		return -1;
	if ((inet_ntop(family, ip, dst, sizeof(dst) - 1)) == NULL)
		return -1;
	*mask = 0;
	if (maskstr != NULL) {
		if (check_ipv4(maskstr)) {
			if (get_netaddr(maskstr, ip) == -1)
				return -1;
			*mask = ip[0];
		} else {
			ret = parse_int(maskstr, (int *)mask);
			if (ret || *mask > 128 || *mask < 0)
				return -1;
			if (family == AF_INET)
				*mask = htonl(~0 << (32 - *mask));
		}
	}

	*ipstr = strdup(dst);
	return 0;
}

int xstrdup(char **dst, const char *src)
{
	char *t;

	if (src == NULL || *dst == src)
		return 0;
	if ((t = strdup(src)) == NULL)
		return vzctl_err(VZCTL_E_NOMEM, ENOMEM, "xstrdup");
	if (*dst != NULL)
		free(*dst);
	*dst = t;
	return 0;
}

static char *unescapestr(char *src)
{
	char *p1, *p2;
	int fl;

	if (src == NULL)
		return NULL;
	p2 = src;
	p1 = p2;
	fl = 0;
	while (*p2) {
		if (*p2 == '\\' && !fl)	{
			fl = 1;
			p2++;
		} else {
			*p1 = *p2;
			p1++; p2++;
			fl = 0;
		}
	}
	*p1 = 0;

	return src;
}

/*
  man bash

  ...a word beginning with # causes that word and all remaining characters on that line to be
  ignored.
 */
static char *uncommentstr(char * str)
{
	char * p1;
	int inb1 = 0, inb2 = 0, inw = 1;

	for(p1 = str; *p1; p1++)
	{
		if ( inb1 && (*p1 != '\'') )
			continue;

		if ( inb2 && (*p1 != '"') )
			continue;

		switch(*p1)
		{
		case '\'':
			inb1 ^= 1;
			inw = 0;
			break;
		case '"':
			inb2 ^= 1;
			inw = 0;
			break;
		case '#':
			if( !inw )
				break;
			*p1 = 0;
			return str;
		default:
			if(isspace(*p1))
				inw = 1;
			else
				inw = 0;
			break;
		}
	}

	return str;
}

char *parse_line(char *str, char *ltoken, int lsz)
{
	char *sp = str;
	char *ep, *p;
	int len;

	unescapestr(str);
	uncommentstr(str);
	while (*sp && isspace(*sp)) sp++;
	if (!*sp || *sp == '#')
		return NULL;
	ep = sp + strlen(sp) - 1;
	while (isspace(*ep) && ep >= sp) *ep-- = '\0';
	if (*ep == '"' || *ep == '\'')
		*ep = 0;
	if (!(p = strchr(sp, '=')))
		return NULL;
	len = p - sp;
	if (len >= lsz)
		return NULL;
	strncpy(ltoken, sp, len);
	ltoken[len] = 0;
	p++;
	if (*p == '"' || *p == '\'' )
		p++;

	return p;
}

/*
	1 - exist
	0 - does't exist
	-1 - error
*/
int stat_file(const char *file)
{
	struct stat st;

	if (stat(file, &st)) {
		if (errno != ENOENT) {
			logger(-1, errno, "Unable to find %s", file);
			return -1;
		}
		return 0;
	}
	return 1;
}

int make_dir(const char *path, int full, int mode)
{
	char buf[4096];
	const char *ps, *p;
	int len;

	if (path == NULL)
		return 0;

	ps = path + 1;
	while ((p = strchr(ps, '/'))) {
		len = p - path + 1;
		snprintf(buf, len, "%s", path);
		ps = p + 1;
		if (!stat_file(buf)) {
			if (mkdir(buf, mode) && errno != EEXIST) {
				logger(-1, errno, "Cannot create the directory %s",
					buf);
				return 1;
			}
		}
	}
	if (!full)
		return 0;
	if (!stat_file(path)) {
		if (mkdir(path, mode) && errno != EEXIST) {
			logger(-1, errno, "Cannot create the directory %s", path);
			return 1;
		}
	}
	return 0;
}

int parse_int(const char *str, int *val)
{
	char *tail;
	long int n;

	if (*str == '\0')
		return 1;

	errno = 0;
	n = strtol(str, (char **)&tail, 10);
	if (*tail != '\0' || errno == ERANGE || n > INT_MAX)
		return 1;
	*val = (int)n;
	return 0;
}

int check_var(const void *val, const char *message)
{
	if (val != NULL)
		return 0;
	logger(0, 0, "%s", message);

	return 1;
}

static int convert_str(const char *to, const char *from, const char *src,
	char *dst, int dst_size)
{
	iconv_t ic;
	char *inptr;
	char *wrptr;
	size_t insize, avail, nconv;
	int ret = 0;

	if (to == NULL || from == NULL || dst_size < 1)
		return 1;

	inptr = (char *)src;
	insize = strlen(src);
	avail = dst_size - 1;
	ic = iconv_open(to, from);
	if (ic == (iconv_t) -1) {
		logger(3, errno, "Error in iconv_open()");
		return 1;
	}
	wrptr = dst;
	nconv = iconv(ic, &inptr, &insize, &wrptr, &avail);
	if (nconv == (size_t) -1) {
		logger(3, errno, "Error in iconv()");
		ret = 1;
	} else {
		iconv(ic, NULL, NULL, &wrptr, &avail);
		*wrptr = 0;
	}
	iconv_close(ic);
	return ret;
}

int vzctl_convertstr(const char *src, char *dst, int dst_size)
{
	setlocale(LC_ALL, "");
	return convert_str(LOCALE_UTF8, nl_langinfo(CODESET), src, dst, dst_size);
}


int utf8tostr(const char *src, char *dst, int dst_size, const char *enc)
{
	char *old_locale = NULL;
	int ret;

	/* Get the name of the current locale. */
	if (enc != NULL)
		old_locale = strdup(setlocale(LC_ALL, NULL));
	else
		setlocale(LC_ALL, "");
	ret = convert_str(enc != NULL ? enc : nl_langinfo(CODESET), LOCALE_UTF8,
		src, dst, dst_size);

	/* Restore the original locale. */
	if (old_locale != NULL) {
		setlocale (LC_ALL, old_locale);
		free(old_locale);
	}
	return ret;
}


static int __copy_file(int fd_src, const char *src,
		int fd_dst, const char *dst)
{
	int ret;
	char buf[4096];

	while(1) {
		ret = read(fd_src, buf, sizeof(buf));
		if (ret == 0)
			break;
		else if (ret < 0) {
			logger(-1, errno, "Unable to read from %s", src);
			ret = -1;
			break;
		}
		if (write(fd_dst, buf, ret) < 0) {
			logger(-1, errno, "Unable to write to %s", dst);
			ret = -1;
			break;
		}
	}

	return ret;
}

int copy_file(const char *src, const char *dst)
{
	int fd_src, fd_dst, ret = 0;
	struct stat st;
	off_t off = 0;
	size_t n;

	logger(3, 0, "copy %s %s", src, dst);
	if (stat(src, &st) < 0) {
		logger(-1, errno, "Unable to find %s", src);
		return -1;
	}
	if ((fd_src = open(src, O_RDONLY)) < 0) {
		logger(-1, errno, "Unable to open %s", src);
		return -1;
	}
	if ((fd_dst = open(dst, O_CREAT| O_TRUNC |O_RDWR, st.st_mode)) < 0) {
		logger(-1, errno, "Unable to open %s", dst);
		close(fd_src);
		return -1;
	}
	n = sendfile(fd_dst, fd_src, &off, st.st_size);
	if (n == -1)
		ret = __copy_file(fd_src, src, fd_dst, dst);
	else if (n != st.st_size) {
		ret = -1;
		logger(-1, 0, "Failed to write to %s:"
				" writen=%lu != total=%lu",
				dst, n, st.st_size);
	}
	if (ret) {
		close(fd_src);
		close(fd_dst);
		unlink(dst);
		return -1;
	}
	fsync(fd_dst);
	if (close(fd_dst)) {
		logger(-1, errno, "Unable to close %s", dst);
		ret = -1;
	}

	close(fd_src);

	return ret;
}

char *arg2str(char **arg)
{
	char **p;
	char *str, *sp;
	int len = 0;

	if (arg == NULL)
		return NULL;
	p = arg;
	while (*p)
		len += strlen(*p++) + 1;
	if ((str = (char *)malloc(len + 1)) == NULL)
		return NULL;
	p = arg;
	sp = str;
	while (*p)
		sp += sprintf(sp, "%s ", *p++);

	return str;
}

void free_ar_str(char **ar)
{
	char **p;

	for (p = ar; *p != NULL; p++) free(*p);
}

inline double max(double val1, double val2)
{
	return (val1 > val2) ? val1 : val2;
}

inline unsigned long max_ul(unsigned long val1, unsigned long val2)
{
	return (val1 > val2) ? val1 : val2;
}

inline unsigned long min_ul(unsigned long val1, unsigned long val2)
{
	return (val1 < val2) ? val1 : val2;
}

/*
 * Reset standard file descriptors to /dev/null in case they are closed.
 */
int reset_std(void)
{
	int ret, i, stdfd;

	stdfd = -1;
	for (i = 0; i < 3; i++) {
		ret = fcntl(i, F_GETFL);
		if (ret < 0 && errno == EBADF) {
			if (stdfd < 0) {
				if ((stdfd = open("/dev/null", O_RDWR)) < 0)
					return -1;
			}
			dup2(stdfd, i);
		}
	}
	return stdfd;
}

int yesno2id(const char *str)
{
	if (!strcmp(str, "yes") || !strcmp(str, "on"))
		return YES;
	else if (!strcmp(str, "no") || !strcmp(str, "off"))
		 return NO;
	return -1;
}

const char *id2yesno(int id)
{
	switch (id) {
	case VZCTL_PARAM_ON:
		return "yes";
	case VZCTL_PARAM_OFF:
		return "no";
	}
	return NULL;
}

int get_net_family(const char *ipstr)
{
	if (strchr(ipstr, ':'))
		return AF_INET6;
	else
		return AF_INET;
}

int get_netaddr(const char *ipstr, unsigned int *ip)
{
	int family;

	family = get_net_family(ipstr);
	if (inet_pton(family, ipstr, ip) <= 0)
		return -1;
	return family;
}

static void free_str_param(struct vzctl_str_param *p)
{
	if (p == NULL)
		return;
	free(p->str);
}

void free_str(list_head_t *head)
{
	struct vzctl_str_param *tmp, *it;

	if (list_empty(head))
		return;
	list_for_each_safe(it, tmp, head, list) {
		free_str_param(it);
		free(it);
	}
	list_head_init(head);
}

struct vzctl_str_param *add_str_param(list_head_t *head, const char *str)
{
	struct vzctl_str_param *p;

	p = malloc(sizeof(struct vzctl_str_param));
	if (p == NULL)
		return NULL;
	if ((p->str = strdup(str)) == NULL) {
		free(p);
		return NULL;
	}
	list_add_tail(&p->list, head);
	return p;
}

char *list2str(const char *prefix, list_head_t *head)
{
	struct vzctl_str_param *it;
	char *str, *sp;
	int len = 0;

	if (prefix!= NULL)
		len += strlen(prefix);

	list_for_each(it, head, list) {
		len += strlen(it->str) + 1;
	}
	str = malloc(len + 1);
	if (str == NULL) {
		logger(-1, ENOMEM, "list2str");
		return NULL;
	}
	*str = '\0';
	sp = str;
	if (prefix != NULL)
		sp += sprintf(sp, "%s", prefix);
	list_for_each(it, head, list) {
		sp += sprintf(sp, "%s ", it->str);
	}
	return str;
}

#define ENV_SIZE 256
FILE *vzctl_popen(char *argv[], char *env[], int quiet)
{
	int child, fd, i, j;
	char *cmd;
	char *envp[ENV_SIZE];
	int out[2];

	if (!stat_file(argv[0])) {
		logger(-1, 0, "executable %s not found", argv[0]);
		return NULL;
	}
	if (pipe(out) == -1) {
		logger(-1, errno, "pipe");
		return NULL;
	}

	cmd = arg2str(argv);
	if (cmd != NULL) {
		logger(2, 0, "running: %s", cmd);
		free(cmd);
	}
	i = 0;
	if (env != NULL) {
		for (i = 0; i < ENV_SIZE - 1 && env[i] != NULL; i++)
			envp[i] = env[i];
	}
	for (j = 0; i < ENV_SIZE - 1 && envp_bash[j] != NULL; i++, j++)
		envp[i] = envp_bash[j];
	envp[i] = NULL;
	if ((child = fork()) == 0) {
		fd = open("/dev/null", O_WRONLY);
		dup2(fd, STDIN_FILENO);
		if (quiet) {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
		} else {
			dup2(out[1], STDOUT_FILENO);
		}
		execve(argv[0], argv, envp);
		logger(-1, errno, "Error exec %s", argv[0]);
		_exit(1);
	} else if (child == -1) {
		logger(-1, errno, "Unable to fork");
		goto err;
	}
	close(out[1]);

	return fdopen(out[0], "r");
err:
	close(out[0]);
	close(out[1]);
	return NULL;
}

int vzctl_pclose(FILE *fp)
{
	int status = 1;

	while (waitpid(-1, &status, 0) == -1)
		if (errno != EINTR)
			break;
	fclose(fp);

	return status;
}

int is_vswap_mode()
{
	return (stat_file("/proc/vz/vswap") == 1);
}

/* fbcdf284-5345-416b-a589-7b5fcaa87673 -> {fbcdf284-5345-416b-a589-7b5fcaa87673} */
int vzctl_get_normalized_guid(const char *str, char *buf, int len)
{
	int i;
	const char *in;
	char *out;

#define UUID_LEN 36
	if (len < UUID_LEN + 3)
		return -1;
	in = (str[0] == '{') ? str + 1 : str;
	buf[0] = '{';
	out = buf + 1;

	for (i = 0; i < UUID_LEN; i++) {
		if (in[i] == '\0')
			break;
		if ((i == 8) || (i == 13) || (i == 18) || (i == 23)) {
			if (in[i] != '-' )
				break;
		} else if (!isxdigit(in[i])) {
			break;
		}
		out[i] = in[i];
	}
	if (i < UUID_LEN)
		return 1;
	if (in[UUID_LEN] != '\0' &&
			(in[UUID_LEN] != '}' || in[UUID_LEN + 1] != '\0'))
		return 1;

	out[UUID_LEN] = '}';
	out[UUID_LEN+1] = '\0';
	return 0;
}

int get_mul(char c, unsigned long long *n)
{
	switch (c) {
	case 'T':
	case 't':
		*n = 1024ll * 1024 * 1024 * 1024;
		break;
	case 'G':
	case 'g':
		*n = 1024 * 1024 * 1024;
		break;
	case 'M':
	case 'm':
		*n = 1024 * 1024;
		break;
	case 'K':
	case 'k':
		*n = 1024;
		break;
	case 'P':
	case 'p':
		*n = 4096;
		break;
	case 'B':
	case 'b':
		*n = 1;
		break;
	default:
		return -1;
	}
	return 0;
}

char *get_ipname(unsigned int ip)
{
	struct in_addr addr;

	addr.s_addr = ip;
	return inet_ntoa(addr);
}
