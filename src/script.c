/* $Id$
 *
 * Copyright (C) SWsoft, 1999-2007. All rights reserved.
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "config.h"

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

